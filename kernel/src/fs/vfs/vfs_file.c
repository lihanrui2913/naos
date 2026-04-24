#include "fs/vfs/vfs_internal.h"
#include "fs/vfs/notify.h"
#include "task/task.h"
#include <mm/cache.h>
#include <mm/mm.h>

static bool vfs_buffer_is_userspace(const void *buf, size_t len) {
    return buf && !check_user_overflow((uint64_t)buf, len);
}

static bool vfs_copy_to_user_or_kernel(void *dst, const void *src, size_t len,
                                       bool dst_is_user) {
    if (len == 0)
        return true;
    if (dst_is_user)
        return !copy_to_user(dst, src, len);
    memcpy(dst, src, len);
    return true;
}

static bool vfs_copy_from_user_or_kernel(void *dst, const void *src, size_t len,
                                         bool src_is_user) {
    if (len == 0)
        return true;
    if (src_is_user)
        return !copy_from_user(dst, src, len);
    memcpy(dst, src, len);
    return true;
}

int vfs_generic_file_readpage(struct vfs_file *file,
                              struct vfs_address_space *mapping, uint64_t index,
                              void *page) {
    struct vfs_inode *inode = mapping->host;
    if (!inode || !file || !file->f_op || !file->f_op->read)
        return -EINVAL;

    uint64_t page_base = index * PAGE_SIZE;
    size_t valid = (size_t)PAGE_SIZE;
    if (page_base >= inode->i_size)
        valid = 0;
    else
        valid = MIN((size_t)PAGE_SIZE, (size_t)(inode->i_size - page_base));

    memset(page, 0, PAGE_SIZE);
    if (valid == 0)
        return 0;

    loff_t pos = (loff_t)page_base;
    ssize_t ret = file->f_op->read(file, page, valid, &pos);
    if (ret < 0)
        return (int)ret;
    return 0;
}

static unsigned int vfs_open_lookup_flags(const struct vfs_open_how *how) {
    unsigned int flags = 0;

    if (!how)
        return 0;

    if (how->resolve & RESOLVE_NO_SYMLINKS)
        flags |= LOOKUP_NO_SYMLINKS;
    if (how->resolve & RESOLVE_BENEATH)
        flags |= LOOKUP_BENEATH;
    if (how->resolve & RESOLVE_IN_ROOT)
        flags |= LOOKUP_IN_ROOT;
    if (how->resolve & RESOLVE_CACHED)
        flags |= LOOKUP_CACHED;
    if (how->resolve & RESOLVE_NO_XDEV)
        flags |= LOOKUP_NO_XDEV;

    return flags;
}

static int vfs_open_last_lookups(int dfd, const char *name,
                                 const struct vfs_open_how *how,
                                 struct vfs_path *parent, struct vfs_qstr *last,
                                 struct vfs_dentry **res_dentry) {
    struct vfs_dentry *dentry;
    unsigned int parent_type = 0;
    unsigned int lookup_flags = LOOKUP_PARENT | vfs_open_lookup_flags(how);
    unsigned int component_flags = LOOKUP_CREATE | vfs_open_lookup_flags(how);
    int ret;

    ret = vfs_path_parent_lookup(dfd, name, lookup_flags, parent, last,
                                 &parent_type);
    if (ret < 0)
        return ret;
    if (!parent->dentry || !parent->dentry->d_inode)
        return -ENOENT;
    if (!S_ISDIR(parent->dentry->d_inode->i_mode))
        return -ENOTDIR;

    dentry = vfs_d_lookup(parent->dentry, last);
    if (dentry) {
        if (dentry->d_op && dentry->d_op->d_revalidate) {
            ret = dentry->d_op->d_revalidate(dentry, component_flags);
            if (ret < 0) {
                vfs_dput(dentry);
                return ret;
            }
            if (ret == 0) {
                vfs_dentry_unhash(dentry);
                vfs_dput(dentry);
                dentry = NULL;
            }
        }
        if (dentry && !dentry->d_inode) {
            vfs_dentry_unhash(dentry);
            vfs_dput(dentry);
            dentry = NULL;
        }
    }

    if (!dentry) {
        struct vfs_inode *dir = parent->dentry->d_inode;

        if (dir->i_op && dir->i_op->lookup) {
            dentry = vfs_d_alloc(parent->dentry->d_sb, parent->dentry, last);
            if (!dentry)
                return -ENOMEM;
            {
                struct vfs_dentry *lookup =
                    dir->i_op->lookup(dir, dentry, component_flags);
                if (IS_ERR(lookup)) {
                    vfs_dput(dentry);
                    return (int)PTR_ERR(lookup);
                }
                if (!lookup) {
                    vfs_dput(dentry);
                    dentry = NULL;
                } else {
                    if (lookup != dentry)
                        vfs_dput(dentry);
                    dentry = lookup;
                }
            }
            if (dentry && !(dentry->d_flags & VFS_DENTRY_HASHED))
                vfs_d_add(parent->dentry, dentry);
        }
    }

    if (!dentry && !(how->flags & O_CREAT))
        return -ENOENT;
    *res_dentry = dentry;
    return 0;
}

struct vfs_file *vfs_alloc_file(const struct vfs_path *path,
                                unsigned int open_flags) {
    struct vfs_file *file;

    if (!path || !path->dentry || !path->dentry->d_inode)
        return NULL;

    file = malloc(sizeof(*file));
    if (!file)
        return NULL;

    memset(file, 0, sizeof(*file));
    vfs_path_get((struct vfs_path *)path);
    file->f_path = *path;
    file->f_inode = vfs_igrab(path->dentry->d_inode);
    file->node = file->f_inode;
    file->f_op = file->f_inode->i_fop;
    file->f_flags = open_flags;
    mutex_init(&file->f_pos_lock);
    spin_init(&file->f_lock);
    vfs_ref_init(&file->f_ref, 1);
    return file;
}

struct vfs_file *vfs_file_get(struct vfs_file *file) {
    if (!file)
        return NULL;
    vfs_ref_get(&file->f_ref);
    return file;
}

void vfs_file_put(struct vfs_file *file) {
    if (!file)
        return;
    if (!vfs_ref_put(&file->f_ref))
        return;

    if (file->f_inode) {
        vfs_bsd_lock_t *flock = &file->f_inode->flock_lock;
        spin_lock(&flock->spin);
        if (flock->owner == (uintptr_t)file) {
            flock->l_type = F_UNLCK;
            flock->owner = 0;
        }
        spin_unlock(&flock->spin);

        spin_lock(&file->f_inode->file_locks_lock);
        vfs_file_lock_t *lock = NULL, *tmp = NULL;
        llist_for_each(lock, tmp, &file->f_inode->file_locks, node) {
            if (!lock->ofd || lock->owner != (uintptr_t)file)
                continue;
            llist_delete(&lock->node);
            free(lock);
        }
        spin_unlock(&file->f_inode->file_locks_lock);
    }

    if (file->f_op && file->f_op->release)
        file->f_op->release(file->f_inode, file);
    if (file->f_inode)
        vfs_iput(file->f_inode);
    vfs_path_put(&file->f_path);
    free(file);
}

void vfs_poll_wait_init(vfs_poll_wait_t *wait, struct task *task,
                        uint32_t events) {
    if (!wait)
        return;

    memset(wait, 0, sizeof(*wait));
    wait->task = task;
    wait->events = events;
    llist_init_head(&wait->node);
}

int vfs_poll_wait_arm(vfs_node_t *node, vfs_poll_wait_t *wait) {
    if (!node || !wait || !wait->task)
        return -EINVAL;
    if (wait->armed)
        return 0;

    wait->watch_node = node;
    wait->revents = 0;

    spin_lock(&node->poll_waiters_lock);
    llist_append(&node->poll_waiters, &wait->node);
    wait->armed = true;
    vfs_igrab(node);
    spin_unlock(&node->poll_waiters_lock);
    return 0;
}

void vfs_poll_wait_disarm(vfs_poll_wait_t *wait) {
    vfs_node_t *node;

    if (!wait || !wait->armed || !wait->watch_node)
        return;

    node = wait->watch_node;
    spin_lock(&node->poll_waiters_lock);
    if (wait->armed) {
        llist_delete(&wait->node);
        wait->armed = false;
        vfs_iput(node);
    }
    spin_unlock(&node->poll_waiters_lock);

    wait->watch_node = NULL;
    llist_init_head(&wait->node);
}

int vfs_poll_wait_sleep(vfs_node_t *node, vfs_poll_wait_t *wait,
                        int64_t timeout_ns, const char *reason) {
    uint32_t want;
    uint64_t deadline = UINT64_MAX;
    bool irq_state;

    if (!node || !wait || !wait->task)
        return -EINVAL;

    want = wait->events | EPOLLERR | EPOLLHUP | EPOLLNVAL | EPOLLRDHUP;
    if (timeout_ns >= 0)
        deadline = nano_time() + (uint64_t)timeout_ns;

    while (true) {
        if (wait->revents & want)
            return EOK;
        if (task_signal_has_deliverable(wait->task))
            return -EINTR;
        if (timeout_ns == 0)
            return ETIMEDOUT;

        int64_t block_ns = -1;
        if (timeout_ns >= 0) {
            uint64_t now = nano_time();
            if (now >= deadline)
                return ETIMEDOUT;
            block_ns = (int64_t)(deadline - now);
        }
        if (block_ns < 0 || block_ns > 10000000LL)
            block_ns = 10000000LL;

        irq_state = arch_interrupt_enabled();
        arch_enable_interrupt();
        int ret = task_block(wait->task, TASK_BLOCKING, block_ns, reason);
        if (!irq_state)
            arch_disable_interrupt();
        if (ret == EOK || ret == ETIMEDOUT)
            continue;
        return ret;
    }
}

void vfs_poll_notify(vfs_node_t *node, uint32_t events) {
    vfs_poll_wait_t *pos, *tmp;

    if (!node || !events)
        return;

    spin_lock(&node->poll_waiters_lock);
    if (events & (EPOLLIN | EPOLLRDNORM | EPOLLRDBAND | EPOLLRDHUP))
        node->poll_seq_in++;
    if (events & (EPOLLOUT | EPOLLWRNORM | EPOLLWRBAND))
        node->poll_seq_out++;
    if (events & (EPOLLPRI | EPOLLMSG))
        node->poll_seq_pri++;

    llist_for_each(pos, tmp, &node->poll_waiters, node) {
        pos->revents |= events;
        if (pos->task)
            task_unblock(pos->task, EOK);
    }
    spin_unlock(&node->poll_waiters_lock);
}

int vfs_openat(int dfd, const char *name, const struct vfs_open_how *how,
               struct vfs_file **out) {
    struct vfs_open_how local_how;
    struct vfs_path parent = {0};
    struct vfs_path target = {0};
    struct vfs_qstr last = {0};
    struct vfs_dentry *dentry = NULL;
    struct vfs_file *file;
    struct vfs_inode *dir;
    struct vfs_path *open_path;
    bool dentry_owned_by_target = false;
    bool created = false;
    int ret;

    if (!name || !out)
        return -EINVAL;

    memset(&local_how, 0, sizeof(local_how));
    if (how)
        local_how = *how;
    if ((local_how.flags & O_CREAT) && current_task && current_task->fs)
        local_how.mode &= ~current_task->fs->umask;

    ret = vfs_open_last_lookups(dfd, name, &local_how, &parent, &last, &dentry);
    if (ret < 0)
        goto out;

    dir = parent.dentry->d_inode;
    if (!dentry || !dentry->d_inode) {
        if (!(local_how.flags & O_CREAT)) {
            ret = -ENOENT;
            goto out;
        }
        if (!dentry) {
            dentry = vfs_d_alloc(parent.dentry->d_sb, parent.dentry, &last);
            if (!dentry) {
                ret = -ENOMEM;
                goto out;
            }
        }
        if (!dir->i_op || !dir->i_op->create) {
            ret = -EOPNOTSUPP;
            goto out;
        }
        ret = dir->i_op->create(dir, dentry, (umode_t)local_how.mode,
                                !!(local_how.flags & O_EXCL));
        if (ret < 0)
            goto out;
        created = true;
        if (!(dentry->d_flags & VFS_DENTRY_HASHED))
            vfs_d_add(parent.dentry, dentry);
    } else if ((local_how.flags & O_CREAT) && (local_how.flags & O_EXCL)) {
        ret = -EEXIST;
        goto out;
    }

    if (!dentry->d_inode) {
        ret = -ENOENT;
        goto out;
    }

    open_path = &(struct vfs_path){.mnt = parent.mnt, .dentry = dentry};
    if (!created) {
        struct vfs_path mounted = {0};

        vfs_path_get(open_path);
        mounted = *open_path;
        vfs_follow_mount(&mounted);
        if (mounted.mnt != parent.mnt || mounted.dentry != dentry) {
            target = mounted;
            open_path = &target;
            dentry = target.dentry;
            dentry_owned_by_target = true;
        } else {
            vfs_path_put(&mounted);
        }
    }
    if (!created && S_ISLNK(dentry->d_inode->i_mode)) {
        if (local_how.resolve & RESOLVE_NO_SYMLINKS) {
            if (!((local_how.flags & O_NOFOLLOW) &&
                  (local_how.flags & O_PATH))) {
                ret = -ELOOP;
                goto out;
            }
        } else if (local_how.flags & O_NOFOLLOW) {
            if (!(local_how.flags & O_PATH)) {
                ret = -ELOOP;
                goto out;
            }
        } else {
            ret = vfs_filename_lookup(
                dfd, name, LOOKUP_FOLLOW | vfs_open_lookup_flags(&local_how),
                &target);
            if (ret < 0)
                goto out;
            if (!target.dentry || !target.dentry->d_inode) {
                ret = -ENOENT;
                goto out;
            }
            open_path = &target;
            dentry = target.dentry;
            dentry_owned_by_target = true;
        }
    }

    if ((local_how.flags & O_DIRECTORY) && !S_ISDIR(dentry->d_inode->i_mode)) {
        ret = -ENOTDIR;
        goto out;
    }
    if (!(local_how.flags & O_PATH) && (local_how.flags & O_TRUNC) &&
        ((local_how.flags & O_ACCMODE_FLAGS) == O_WRONLY ||
         (local_how.flags & O_ACCMODE_FLAGS) == O_RDWR) &&
        !S_ISDIR(dentry->d_inode->i_mode)) {
        ret = vfs_truncate_path(open_path, 0);
        if (ret < 0)
            goto out;
    }

    file = vfs_alloc_file(open_path, (unsigned int)local_how.flags);
    if (!file) {
        ret = -ENOMEM;
        goto out;
    }

    if (!(local_how.flags & O_PATH) && dentry->d_inode->i_op &&
        dentry->d_inode->i_op->atomic_open) {
        ret = dentry->d_inode->i_op->atomic_open(dir, dentry, file,
                                                 (unsigned int)local_how.flags,
                                                 (umode_t)local_how.mode);
        if (ret < 0) {
            vfs_file_put(file);
            goto out;
        }
    } else if (!(local_how.flags & O_PATH) && file->f_op && file->f_op->open) {
        ret = file->f_op->open(dentry->d_inode, file);
        if (ret < 0) {
            vfs_file_put(file);
            goto out;
        }
    }

    if (created)
        notifyfs_queue_inode_event(dir, dentry->d_inode, last.name, IN_CREATE,
                                   0);

    if (!(local_how.flags & O_PATH))
        notifyfs_queue_inode_event(dentry->d_inode, dentry->d_inode, NULL,
                                   IN_OPEN, 0);
    *out = file;
    ret = 0;

out:
    vfs_path_put(&target);
    if (dentry && !dentry_owned_by_target)
        vfs_dput(dentry);
    vfs_path_put(&parent);
    vfs_qstr_destroy(&last);
    return ret;
}

int vfs_close_file(struct vfs_file *file) {
    uint64_t close_mask;

    if (!file)
        return -EBADF;
    if (file->f_op && file->f_op->flush)
        file->f_op->flush(file);
    close_mask = ((file->f_flags & O_ACCMODE_FLAGS) == O_RDONLY)
                     ? IN_CLOSE_NOWRITE
                     : IN_CLOSE_WRITE;
    notifyfs_queue_inode_event(file->f_inode, file->f_inode, NULL, close_mask,
                               0);
    vfs_file_put(file);
    return 0;
}

static bool vfs_file_has_pagecache(struct vfs_file *file) {
    return file && file->f_inode && file->f_inode->i_mapping.a_ops &&
           file->f_inode->i_mapping.a_ops->readpage;
}

static ssize_t vfs_file_read_pagecache(struct vfs_file *file, void *buf,
                                       size_t count, loff_t pos) {
    struct vfs_inode *inode = file->f_inode;
    uint64_t i_size = inode->i_size;

    if ((uint64_t)pos >= i_size)
        return 0;

    count = MIN(count, (size_t)(i_size - (uint64_t)pos));
    uint8_t *dst = (uint8_t *)buf;
    bool dst_is_user = vfs_buffer_is_userspace(buf, count);
    size_t read_total = 0;

    while (read_total < count) {
        uint64_t page_index = (uint64_t)pos / PAGE_SIZE;
        uint64_t page_offset = (uint64_t)pos % PAGE_SIZE;
        uint64_t chunk = MIN((uint64_t)PAGE_SIZE - page_offset,
                             (uint64_t)(count - read_total));
        bool created = false;

        cache_entry_t *entry =
            cache_page_get_or_create(inode, page_index, &created);
        if (!entry)
            return read_total > 0 ? (ssize_t)read_total : -ENOMEM;

        if (created) {
            int ret = inode->i_mapping.a_ops->readpage(
                file, &inode->i_mapping, page_index, cache_entry_data(entry));
            if (ret < 0) {
                cache_entry_abort_fill(entry);
                return read_total > 0 ? (ssize_t)read_total : ret;
            }
            cache_entry_mark_ready(entry, PAGE_SIZE);
        }

        size_t valid = cache_entry_valid_bytes(entry);
        if (valid == 0 || page_offset >= valid) {
            cache_entry_put(entry);
            break;
        }
        chunk = MIN(chunk, valid - page_offset);

        void *src = (uint8_t *)cache_entry_data(entry) + page_offset;
        if (!vfs_copy_to_user_or_kernel(dst, src, (size_t)chunk, dst_is_user)) {
            cache_entry_put(entry);
            return read_total > 0 ? (ssize_t)read_total : -EFAULT;
        }

        cache_entry_put(entry);
        dst += chunk;
        pos += (loff_t)chunk;
        read_total += (size_t)chunk;
    }

    if (read_total > 0) {
        uint64_t next_index = (uint64_t)pos / PAGE_SIZE;
        uint64_t next_page_base = next_index * PAGE_SIZE;
        if (next_page_base < i_size) {
            cache_entry_t *prefetch_entry =
                cache_page_try_get(inode, next_index);
            if (!prefetch_entry) {
                prefetch_entry =
                    cache_page_get_or_create(inode, next_index, NULL);
                if (prefetch_entry) {
                    inode->i_mapping.a_ops->readpage(
                        file, &inode->i_mapping, next_index,
                        cache_entry_data(prefetch_entry));
                    cache_entry_mark_ready(prefetch_entry, PAGE_SIZE);
                    cache_entry_put(prefetch_entry);
                }
            } else {
                cache_entry_put(prefetch_entry);
            }
        }
    }

    return (ssize_t)read_total;
}

static ssize_t vfs_file_write_pagecache(struct vfs_file *file, const void *buf,
                                        size_t count, loff_t pos) {
    struct vfs_inode *inode = file->f_inode;
    const uint8_t *src = (const uint8_t *)buf;
    bool src_is_user = vfs_buffer_is_userspace(buf, count);
    size_t write_total = 0;

    while (write_total < count) {
        uint64_t page_index = (uint64_t)pos / PAGE_SIZE;
        uint64_t page_offset = (uint64_t)pos % PAGE_SIZE;
        uint64_t chunk = MIN((uint64_t)PAGE_SIZE - page_offset,
                             (uint64_t)(count - write_total));
        bool created = false;

        cache_entry_t *entry =
            cache_page_get_or_create(inode, page_index, &created);
        if (!entry)
            return write_total > 0 ? (ssize_t)write_total : -ENOMEM;

        if (created && chunk < PAGE_SIZE) {
            if (inode->i_mapping.a_ops->readpage(file, &inode->i_mapping,
                                                 page_index,
                                                 cache_entry_data(entry)) < 0) {
                cache_entry_abort_fill(entry);
                return write_total > 0 ? (ssize_t)write_total : -EIO;
            }
            cache_entry_mark_ready(entry, PAGE_SIZE);
        } else if (created) {
            cache_entry_mark_ready(entry, PAGE_SIZE);
        }

        void *dst = (uint8_t *)cache_entry_data(entry) + page_offset;
        if (!vfs_copy_from_user_or_kernel(dst, src, (size_t)chunk,
                                          src_is_user)) {
            cache_entry_put(entry);
            return write_total > 0 ? (ssize_t)write_total : -EFAULT;
        }

        cache_page_mark_dirty(entry);
        cache_entry_put(entry);

        src += chunk;
        pos += (loff_t)chunk;
        write_total += (size_t)chunk;
    }

    if ((uint64_t)pos > inode->i_size)
        inode->i_size = (uint64_t)pos;

    return (ssize_t)write_total;
}

ssize_t vfs_read_file(struct vfs_file *file, void *buf, size_t count,
                      loff_t *ppos) {
    ssize_t ret;
    loff_t pos;

    if (!file || !file->f_op || !file->f_op->read)
        return -EINVAL;

    if (ppos) {
        pos = *ppos;
        if (vfs_file_has_pagecache(file)) {
            ret = vfs_file_read_pagecache(file, buf, count, pos);
            if (ret >= 0)
                *ppos = (loff_t)((uint64_t)pos + (uint64_t)ret);
        } else {
            ret = file->f_op->read(file, buf, count, &pos);
            if (ret >= 0)
                *ppos = pos;
        }
        return ret;
    }

    mutex_lock(&file->f_pos_lock);
    pos = file->f_pos;
    if (vfs_file_has_pagecache(file)) {
        ret = vfs_file_read_pagecache(file, buf, count, pos);
        if (ret >= 0)
            file->f_pos = (loff_t)((uint64_t)pos + (uint64_t)ret);
    } else {
        ret = file->f_op->read(file, buf, count, &pos);
        if (ret >= 0)
            file->f_pos = pos;
    }
    mutex_unlock(&file->f_pos_lock);
    return ret;
}

ssize_t vfs_write_file(struct vfs_file *file, const void *buf, size_t count,
                       loff_t *ppos) {
    ssize_t ret;
    loff_t pos;

    if (!file || !file->f_op || !file->f_op->write)
        return -EINVAL;

    if (ppos) {
        pos = *ppos;
        if (vfs_file_has_pagecache(file)) {
            ret = vfs_file_write_pagecache(file, buf, count, pos);
            if (ret >= 0) {
                *ppos = (loff_t)((uint64_t)pos + (uint64_t)ret);
                if (ret > 0)
                    notifyfs_queue_inode_event(file->f_inode, file->f_inode,
                                               NULL, IN_MODIFY, 0);
            }
        } else {
            ret = file->f_op->write(file, buf, count, &pos);
            if (ret >= 0) {
                *ppos = pos;
                if (ret > 0)
                    notifyfs_queue_inode_event(file->f_inode, file->f_inode,
                                               NULL, IN_MODIFY, 0);
            }
        }
        return ret;
    }

    mutex_lock(&file->f_pos_lock);
    pos = file->f_pos;
    if (file->f_flags & O_APPEND)
        pos = (loff_t)file->f_inode->i_size;
    if (vfs_file_has_pagecache(file)) {
        ret = vfs_file_write_pagecache(file, buf, count, pos);
        if (ret >= 0) {
            file->f_pos = (loff_t)((uint64_t)pos + (uint64_t)ret);
            if (ret > 0)
                notifyfs_queue_inode_event(file->f_inode, file->f_inode, NULL,
                                           IN_MODIFY, 0);
        }
    } else {
        ret = file->f_op->write(file, buf, count, &pos);
        if (ret >= 0) {
            file->f_pos = pos;
            if (ret > 0)
                notifyfs_queue_inode_event(file->f_inode, file->f_inode, NULL,
                                           IN_MODIFY, 0);
        }
    }
    mutex_unlock(&file->f_pos_lock);
    return ret;
}

loff_t vfs_llseek_file(struct vfs_file *file, loff_t offset, int whence) {
    loff_t new_pos;

    if (!file)
        return -EBADF;
    if (file->f_op && file->f_op->llseek)
        return file->f_op->llseek(file, offset, whence);

    mutex_lock(&file->f_pos_lock);
    switch (whence) {
    case SEEK_SET:
        new_pos = offset;
        break;
    case SEEK_CUR:
        new_pos = file->f_pos + offset;
        break;
    case SEEK_END:
        new_pos = (loff_t)file->f_inode->i_size + offset;
        break;
    default:
        mutex_unlock(&file->f_pos_lock);
        return -EINVAL;
    }

    if (new_pos < 0) {
        mutex_unlock(&file->f_pos_lock);
        return -EINVAL;
    }

    file->f_pos = new_pos;
    mutex_unlock(&file->f_pos_lock);
    return new_pos;
}

int vfs_iterate_dir(struct vfs_file *file, struct vfs_dir_context *ctx) {
    if (!file || !ctx)
        return -EINVAL;
    if (!file->f_op || !file->f_op->iterate_shared)
        return -ENOTDIR;
    return file->f_op->iterate_shared(file, ctx);
}

long vfs_ioctl_file(struct vfs_file *file, unsigned long cmd,
                    unsigned long arg) {
    if (!file)
        return -EBADF;
    if (!file->f_op || !file->f_op->unlocked_ioctl)
        return -ENOTTY;
    return file->f_op->unlocked_ioctl(file, cmd, arg);
}

int vfs_fsync_file(struct vfs_file *file) {
    if (!file)
        return -EBADF;
    if (!file->f_op || !file->f_op->fsync)
        return 0;
    return file->f_op->fsync(file, 0, (loff_t)file->f_inode->i_size, 0);
}

int vfs_poll(vfs_node_t *node, size_t events) {
    struct vfs_file fake = {0};

    if (!node || !node->i_fop || !node->i_fop->poll)
        return -ENOTSUP;

    fake.f_op = node->i_fop;
    fake.f_inode = node;
    fake.node = node;
    return (int)node->i_fop->poll(&fake, NULL) & (int)events;
}

int vfs_truncate_path(const struct vfs_path *path, uint64_t size) {
    struct vfs_kstat stat;
    struct vfs_inode *inode;
    uint64_t old_size;
    int ret;

    if (!path || !path->dentry || !path->dentry->d_inode)
        return -ENOENT;

    inode = path->dentry->d_inode;
    if (!inode->i_op || !inode->i_op->setattr)
        return -EOPNOTSUPP;

    old_size = inode->i_size;
    vfs_fill_generic_kstat(path, &stat);
    stat.size = size;

    ret = inode->i_op->setattr(path->dentry, &stat);
    if (ret == 0) {
        uint64_t first = (size + PAGE_SIZE - 1) / PAGE_SIZE;
        uint64_t last = (old_size > 0) ? ((old_size - 1) / PAGE_SIZE) : 0;
        for (uint64_t p = first; p <= last; p++)
            cache_page_invalidate_range(inode, p * PAGE_SIZE, PAGE_SIZE);
        notifyfs_queue_inode_event(inode, inode, NULL, IN_MODIFY, 0);
    }
    return ret;
}
