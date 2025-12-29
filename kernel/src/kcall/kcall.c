#include <kcall/kcall.h>
#include <kcall/handle.h>
#include <task/universe.h>
#include <task/task.h>
#include <task/sched.h>

#define USER_MMAP_START 0x0000100000000000
#define USER_MMAP_END 0x0000600000000000

static uint64_t find_unmapped_area(vma_manager_t *mgr, uint64_t hint,
                                   uint64_t len) {
    vma_t *vma;

    // 参数校验
    if (len == 0 || len > USER_MMAP_END - USER_MMAP_START) {
        return (uint64_t)-ENOMEM;
    }

    // 尝试hint
    if (hint) {
        hint = PADDING_UP(hint, DEFAULT_PAGE_SIZE);
        if (hint >= USER_MMAP_START && hint <= USER_MMAP_END - len &&
            !vma_find_intersection(mgr, hint, hint + len)) {
            return hint;
        }
    }

    // 使用红黑树找到第一个VMA
    rb_node_t *node = rb_first(&mgr->vma_tree);

    if (!node) {
        // 没有VMA，整个空间可用
        return USER_MMAP_START + len <= USER_MMAP_END ? USER_MMAP_START
                                                      : (uint64_t)-ENOMEM;
    }

    vma = rb_entry(node, vma_t, vm_rb);

    // 检查第一个VMA之前的gap
    if (vma->vm_start >= USER_MMAP_START + len) {
        return USER_MMAP_START;
    }

    // 扫描VMA间的gaps
    while ((node = rb_next(node)) != NULL) {
        vma_t *next_vma = rb_entry(node, vma_t, vm_rb);
        uint64_t gap_start = vma->vm_end;
        uint64_t gap_end = next_vma->vm_start;

        if (gap_end >= gap_start + len) {
            return gap_start;
        }

        vma = next_vma;
    }

    return (uint64_t)-ENOMEM;
}

universe_t *get_universe(handle_id_t handle_id) {
    if (handle_id == kThisUniverse) {
        return current_task->universe;
    }
    if (handle_id < 0 || handle_id > current_task->universe->max_handle_count)
        return NULL;
    if (!current_task->universe->handles[handle_id])
        return NULL;
    if (current_task->universe->handles[handle_id]->handle_type != UNIVERSE)
        return NULL;
    return current_task->universe->handles[handle_id]->universe.universe;
}

uint64_t *get_space(handle_id_t handle_id) {
    if (handle_id == kThisSpace) {
        return phys_to_virt(current_task->mm->page_table_addr);
    }
    if (handle_id < 0 || handle_id > current_task->universe->max_handle_count)
        return NULL;
    if (!current_task->universe->handles[handle_id])
        return NULL;
    if (current_task->universe->handles[handle_id]->handle_type != SPACE)
        return NULL;
    return phys_to_virt(current_task->universe->handles[handle_id]
                            ->space.space->page_table_addr);
}

bool read_user_memory(void *kptr, const void *uptr, size_t size) {
    uintptr_t limit;
    if (__builtin_add_overflow((uint64_t)uptr, size, &limit))
        return false;
    bool fault = copy_from_user(kptr, uptr, size);
    return !fault;
}

bool write_user_memory(void *uptr, const void *kptr, size_t size) {
    uintptr_t limit;
    if (__builtin_add_overflow((uint64_t)uptr, size, &limit))
        return false;
    bool fault = copy_to_user(uptr, kptr, size);
    return !fault;
}

#define LOG_LINE_LENGTH 256

k_error_t kCallLogImpl(enum kLogSeverity severity, const char *string,
                       size_t length) {
    size_t offset = 0;
    while (offset < length) {
        char log[LOG_LINE_LENGTH] = {0};
        size_t chunk = MIN(length - offset, LOG_LINE_LENGTH);

        if (!read_user_memory(log, string + offset, chunk))
            return kErrFault;

        switch (severity) {
        case kLogSeverityEmergency:
        case kLogSeverityAlert:
        case kLogSeverityCritical:
        case kLogSeverityError: {
            printk("[LOG] %s", log);
            break;
        }
        case kLogSeverityWarning: {
            printk("[WARN] %s", log);
            break;
        }
        case kLogSeverityNotice:
        case kLogSeverityInfo: {
            printk("[INFO] %s", log);
            break;
        }
        case kLogSeverityDebug: {
            printk("[DEBUG] %s", log);
            break;
        }
        default:
            break;
        }

        offset += chunk;
    }

    return kErrNone;
}

void kCallPanicImpl(const char *string, size_t length) {
    size_t offset = 0;
    while (offset < length) {
        char log[LOG_LINE_LENGTH] = {0};
        size_t chunk = MIN(length - offset, LOG_LINE_LENGTH);

        if (!read_user_memory(log, string + offset, chunk))
            return;

        printk("[PANIC]: %s", log);

        offset += chunk;
    }

    task_exit(0);
}

k_error_t kCallNopImpl() {
    arch_pause();
    return kErrNone;
}

k_error_t kCallGetRandomBytesImpl(void *buf, size_t wanted_size,
                                  size_t *actual_size) {
    for (int i = 0; i < wanted_size; i++) {
        tm time;
        time_read(&time);
        uint8_t byte = (uint8_t)(mktime(&time) * 114514 % 1919810);
        if (!write_user_memory(buf, &byte, sizeof(byte))) {
            return kErrFault;
        }
    }

    *actual_size = wanted_size;
    return kErrNone;
}

k_error_t kCallGetClockImpl(int64_t *clock) {
    uint64_t nano = nano_time();
    if (!write_user_memory(clock, &nano, sizeof(int64_t)))
        return kErrFault;
    return kErrNone;
}

k_error_t kCreateUniverseImpl(handle_id_t *out) {
    handle_t *handle = malloc(sizeof(handle_t));
    handle->refcount = 1;
    handle->handle_type = UNIVERSE;
    handle->universe.universe = create_universe();
    handle_id_t id = attach_handle(current_task->universe, handle);
    if (K_RES_IS_ERR(id)) {
        return kErrOutOfBounds;
    }
    if (!write_user_memory(out, &id, sizeof(handle_id_t)))
        return kErrFault;
    return kErrNone;
}

k_error_t kTransferDescriptorImpl(handle_id_t handle_id,
                                  handle_id_t universe_handle,
                                  enum kTransferDescriptorFlags dir,
                                  handle_id_t *out_handle) {
    universe_t *this_universe = current_task->universe;
    universe_t *another_uinverse = get_universe(universe_handle);
    if (!another_uinverse) {
        return kErrBadDescriptor;
    }

    universe_t *dst, *src;
    if (dir == kTransferDescriptorOut) {
        src = this_universe;
        dst = another_uinverse;
    } else {
        src = another_uinverse;
        dst = this_universe;
    }

    if (handle_id < 0 || handle_id > src->max_handle_count)
        return kErrBadDescriptor;
    handle_t *handle = src->handles[handle_id];
    if (!handle) {
        return kErrBadDescriptor;
    }

    handle_id_t out_handle_id = attach_handle(dst, handle);

    *out_handle = out_handle_id;

    return kErrNone;
}

k_error_t kGetDescriptorInfoImpl(handle_id_t handle_id,
                                 k_descriptor_info_t *info) {
    if (handle_id < 0 || handle_id > current_task->universe->max_handle_count)
        return kErrBadDescriptor;
    handle_t *handle = current_task->universe->handles[handle_id];
    if (!handle) {
        return kErrBadDescriptor;
    }
    info->type = handle->handle_type;
    return kErrNone;
}

k_error_t kCloseDescriptorImpl(handle_id_t universe_handle,
                               handle_id_t handle_id) {
    universe_t *universe = get_universe(universe_handle);
    if (handle_id < 0 || handle_id > universe->max_handle_count)
        return kErrBadDescriptor;
    handle_t *handle = universe->handles[handle_id];
    if (!handle) {
        return kErrBadDescriptor;
    }
    // TODO: Free inner handles
    detatch_handle(universe, handle_id);
    free(handle);
    return kErrNone;
}

struct llist_header futexing_tasks = {
    .prev = &futexing_tasks,
    .next = &futexing_tasks,
};

k_error_t kFutexWaitImpl(int *pointer, int except, int64_t deadline) {
    int key;
    if (!read_user_memory(&key, pointer, sizeof(int)))
        return kErrFault;
    if (key != except)
        return kErrCancelled;
    current_task->futex_pointer = pointer;
    task_block(current_task, &futexing_tasks, deadline, __func__);
    return kErrNone;
}

k_error_t kFutexWakeImpl(int *pointer, int count) {
    task_t *task, *tmp;
    int wakeup = 0;
    llist_for_each(task, tmp, &futexing_tasks, node) {
        if (wakeup >= count)
            break;
        if (task->futex_pointer == pointer) {
            task_unblock(task);
            wakeup++;
        }
    }
    return kErrNone;
}

k_error_t kAllocateMemoryImpl(size_t size, uint32_t flags,
                              const k_allocate_restrictions_t *res,
                              handle_id_t *out) {
    k_allocate_restrictions_t r;
    if (!read_user_memory(&r, res, sizeof(k_allocate_restrictions_t)))
        return kErrFault;
    size = PADDING_UP(size, DEFAULT_PAGE_SIZE);
    size_t pages = size / DEFAULT_PAGE_SIZE;
    handle_t *handle = malloc(sizeof(handle_t));
    handle->refcount = 1;
    handle->handle_type = MEMORY;
    if (r.address_bits <= 32) {
        handle->memory.address = alloc_frames_dma32(pages);
    } else {
        handle->memory.address = alloc_frames(pages);
    }
    handle->memory.len = size;
    handle->memory.info = flags;
    handle_id_t id = attach_handle(current_task->universe, handle);
    if (!write_user_memory(out, &id, sizeof(handle_id_t))) {
        detatch_handle(current_task->universe, id);
        free(handle);
        return kErrFault;
    }

    return kErrNone;
}

k_error_t kResizeMemoryImpl(handle_id_t handle_id, size_t new_size) {
    if (handle_id < 0 || handle_id > current_task->universe->max_handle_count)
        return kErrBadDescriptor;
    handle_t *handle = current_task->universe->handles[handle_id];
    if (!handle) {
        return kErrBadDescriptor;
    }
    if (handle->handle_type != MEMORY) {
        return kErrBadDescriptor;
    }
    new_size = PADDING_UP(new_size, DEFAULT_PAGE_SIZE);
    size_t new_pages = new_size / DEFAULT_PAGE_SIZE;
    size_t old_pages = handle->memory.len / DEFAULT_PAGE_SIZE;
    uintptr_t new = alloc_frames(new_pages);
    memcpy(phys_to_virt((void *)new),
           phys_to_virt((void *)handle->memory.address),
           MIN(new_pages, old_pages));
    free_frames(handle->memory.address, old_pages);
    handle->memory.address = new;
    handle->memory.len = new_size;
    return kErrNone;
}

k_error_t kGetMemoryInfoImpl(handle_id_t handle_id, size_t *len, size_t *info) {
    if (handle_id < 0 || handle_id > current_task->universe->max_handle_count)
        return kErrBadDescriptor;
    handle_t *handle = current_task->universe->handles[handle_id];
    if (!handle) {
        return kErrBadDescriptor;
    }
    if (handle->handle_type != MEMORY) {
        return kErrBadDescriptor;
    }
    if (!write_user_memory(len, &handle->memory.len, sizeof(size_t))) {
        return kErrFault;
    }
    if (!write_user_memory(info, &handle->memory.info, sizeof(size_t))) {
        return kErrFault;
    }
    return kErrNone;
}

k_error_t kSetMemoryInfoImpl(handle_id_t handle_id, size_t info) {
    if (handle_id < 0 || handle_id > current_task->universe->max_handle_count)
        return kErrBadDescriptor;
    handle_t *handle = current_task->universe->handles[handle_id];
    if (!handle) {
        return kErrBadDescriptor;
    }
    if (handle->handle_type != MEMORY) {
        return kErrBadDescriptor;
    }
    handle->memory.info = info;
    return kErrNone;
}

k_error_t kMapMemoryImpl(handle_id_t memory_handle, handle_id_t space_id,
                         void *pointer, size_t size, uint32_t flags,
                         void **actual_pointer) {
    if (memory_handle < 0 ||
        memory_handle > current_task->universe->max_handle_count)
        return kErrBadDescriptor;
    handle_t *handle = current_task->universe->handles[memory_handle];
    if (!handle) {
        return kErrBadDescriptor;
    }
    if (handle->handle_type != MEMORY) {
        return kErrBadDescriptor;
    }
    uint64_t *space = get_space(space_id);
    if (!space) {
        return kErrBadDescriptor;
    }
    if (!pointer) {
        pointer = (void *)find_unmapped_area(&current_task->mm->task_vma_mgr,
                                             (uint64_t)pointer, size);
    }
    if (!write_user_memory(actual_pointer, &pointer, sizeof(void *))) {
        return kErrFault;
    }
    map_page_range(space, (uint64_t)pointer, handle->memory.address, size,
                   handle->memory.info);
    return kErrNone;
}

k_error_t kUnmapMemoryImpl(handle_id_t memory_handle, void *pointer,
                           size_t size) {
    if (memory_handle < 0 ||
        memory_handle > current_task->universe->max_handle_count)
        return kErrBadDescriptor;
    handle_t *handle = current_task->universe->handles[memory_handle];
    if (!handle) {
        return kErrBadDescriptor;
    }
    if (handle->handle_type != MEMORY) {
        return kErrBadDescriptor;
    }
    unmap_page_range(get_current_page_dir(false), (uint64_t)pointer, size);
    return kErrNone;
}

k_error_t kCreatePhysicalMemoryImpl(uintptr_t paddr, size_t size, size_t info,
                                    handle_id_t *out) {
    size = PADDING_UP(size, DEFAULT_PAGE_SIZE);
    size_t pages = size / DEFAULT_PAGE_SIZE;
    handle_t *handle = malloc(sizeof(handle_t));
    handle->refcount = 1;
    handle->handle_type = MEMORY;
    handle->memory.address = paddr;
    handle->memory.len = size;
    handle->memory.info = 0;
    handle_id_t id = attach_handle(current_task->universe, handle);
    if (!write_user_memory(out, &id, sizeof(handle_id_t))) {
        detatch_handle(current_task->universe, id);
        free(handle);
        return kErrFault;
    }
    return kErrNone;
}

k_error_t kCreateStreamImpl(handle_id_t *handle_out1,
                            handle_id_t *handle_out2) {
    lane_t *lane1 = malloc(sizeof(lane_t));
    spin_init(&lane1->lock);
    lane1->recv_pos = 0;
    lane1->recv_size = 0;
    lane1->recv_buff = calloc(1, LANE_BUFFER_SIZE);
    lane1->pending_descs = calloc(LANE_PENDING_DESC_NUM, sizeof(handle_t *));

    lane_t *lane2 = malloc(sizeof(lane_t));
    spin_init(&lane2->lock);
    lane2->recv_pos = 0;
    lane2->recv_size = 0;
    lane2->recv_buff = calloc(1, LANE_BUFFER_SIZE);
    lane2->pending_descs = calloc(LANE_PENDING_DESC_NUM, sizeof(handle_t *));

    lane2->peer = lane1;
    lane2->peer_connected = true;
    lane1->peer = lane2;
    lane1->peer_connected = true;

    handle_t *handle1 = malloc(sizeof(handle_t));
    handle1->refcount = 1;
    handle1->handle_type = LANE;
    handle1->lane.lane = lane1;

    handle_t *handle2 = malloc(sizeof(handle_t));
    handle2->refcount = 1;
    handle2->handle_type = LANE;
    handle2->lane.lane = lane2;

    handle_id_t id1 = attach_handle(current_task->universe, handle1);
    handle_id_t id2 = attach_handle(current_task->universe, handle2);

    *handle_out1 = id1;
    *handle_out2 = id2;

    return kErrNone;
}

k_error_t kCreateSpaceImpl(handle_id_t *out) {
    handle_t *handle = malloc(sizeof(handle_t));
    handle->refcount = 1;
    handle->handle_type = SPACE;
    handle->space.space = clone_page_table(get_current_page_dir(false), 0);

    handle_id_t id = attach_handle(current_task->universe, handle);

    *out = id;

    return kErrNone;
}

k_error_t kCreateThreadImpl(handle_id_t universe_id, handle_id_t space_id,
                            void *ip, void *sp, uint64_t arg,
                            handle_id_t *thread_handle_out) {
    universe_t *universe = get_universe(universe_id);
    if (!universe) {
        return kErrBadDescriptor;
    }
    uint64_t *page_table_addr = get_space(space_id);
    if (!page_table_addr) {
        return kErrBadDescriptor;
    }

    handle_t *handle = malloc(sizeof(handle_t));
    handle->refcount = 1;
    handle->handle_type = THREAD;
    handle_id_t id = attach_handle(current_task->universe, handle);
    if (K_RES_IS_ERR(id)) {
        return kErrOutOfBounds;
    }
    if (!write_user_memory(thread_handle_out, &id, sizeof(handle_id_t))) {
        detatch_handle(current_task->universe, id);
        free(handle);
        return kErrFault;
    }
    task_t *task = task_create_user(universe, page_table_addr, ip, sp, arg, 0);
    handle->thread.task = task;

    return kErrNone;
}

k_error_t kLoadRegistersImpl(handle_id_t handle, int set, void *image) {}

k_error_t kStoreRegistersImpl(handle_id_t handle, int set, const void *image) {}

k_error_t kSubmitDescriptorImpl(handle_id_t handle_id, k_action_t *action,
                                size_t count, uint32_t flags) {
    k_action_t actions[128];

    if (handle_id < 0 || handle_id > current_task->universe->max_handle_count)
        return kErrBadDescriptor;
    handle_t *handle = current_task->universe->handles[handle_id];
    if (!handle) {
        return kErrBadDescriptor;
    }
    if (handle->handle_type != LANE) {
        return kErrBadDescriptor;
    }

    lane_t *self = handle->lane.lane;
    if (!self) {
        return kErrBadDescriptor;
    }

    if (!read_user_memory(actions, action, sizeof(k_action_t) * count))
        return kErrFault;

    size_t recv_pos =
        (self->peer && self->peer->peer_connected) ? self->peer->recv_pos : 0;
    for (size_t i = 0; i < count; i++) {
        k_action_t *recipe = &actions[i];

        switch (recipe->type) {
        case kActionDismiss:
            break;
        case kActionOffer:
            handle_t *remote_handle =
                current_task->universe->handles[recipe->handle];
            if (remote_handle->handle_type != LANE)
                continue;
            remote_handle->refcount++;
            detatch_handle(current_task->universe, recipe->handle);
            lane_t *remote_lane = remote_handle->lane.lane;
            spin_lock(&self->peer->lock);
            for (int i = 0; i < LANE_MAX_CONNECTIONS; i++) {
                if (!self->peer->connections[i]) {
                    self->peer->connections[i] = remote_lane;
                    break;
                }
            }
            spin_unlock(&self->peer->lock);
            break;
        case kActionAccept:
            break;
        case kActionSendFromBuffer:
            if (!self->peer || !self->peer->peer_connected) {
                return kErrDismissed;
            }
            // TODO: Now we ensure the length < recv_size
            spin_lock(&self->peer->lock);
            read_user_memory(self->peer->recv_buff + recv_pos, action->buffer,
                             action->length);
            recv_pos += action->length;
            spin_unlock(&self->peer->lock);
            break;
        case kActionRecvToBuffer:
            break;
        case kActionPushDescriptor:
            if (!self->peer || !self->peer->peer_connected) {
                return kErrDismissed;
            }
            spin_lock(&self->peer->lock);
            handle_t *handle = current_task->universe->handles[action->handle];
            for (int i = 0; i < LANE_PENDING_DESC_NUM; i++) {
                if (!self->peer->pending_descs[i]) {
                    handle->refcount++;
                    self->peer->pending_descs[i] = handle;
                    break;
                }
            }
            spin_unlock(&self->peer->lock);
            break;
        case kActionPullDescriptor:
            break;
        default:
            break;
        }
    }
    if (self->peer && self->peer->peer_connected) {
        self->peer->recv_pos = recv_pos;
    }

    if (flags & KCALL_SUBMIT_NO_RECEIVING) {
        return kErrNone;
    }

    while (self->peer ? !self->recv_pos : self->connections[0]) {
        schedule(SCHED_YIELD);
    }

    recv_pos = self->recv_pos;
    for (size_t i = 0; i < count; i++) {
        k_action_t *recipe = &actions[i];

        switch (recipe->type) {
        case kActionDismiss:
            break;
        case kActionOffer:
            break;
        case kActionAccept:
            if (self->connections[0]) {
                lane_t *remote = self->connections[0];

                handle_t *handle = malloc(sizeof(handle_t));
                handle->refcount = 1;
                handle->handle_type = LANE;
                handle->lane.lane = remote;

                recipe->handle = attach_handle(current_task->universe, handle);

                memmove(self->connections, &self->connections[1],
                        sizeof(struct lane *) * (LANE_MAX_CONNECTIONS - 1));
            }
            break;
        case kActionSendFromBuffer:
            break;
        case kActionRecvToBuffer:
            if (!recv_pos) {
                return kErrEndOfLane;
            }
            if (recipe->length > self->recv_size) {
                return kErrBufferTooSmall;
            }
            spin_lock(&self->lock);
            memcpy(recipe->buffer, self->recv_buff + recv_pos, recv_pos);
            memmove(self->recv_buff, &self->recv_buff[recv_pos], recv_pos);
            recipe->length = recv_pos;
            recv_pos = 0;
            spin_unlock(&self->lock);
            break;
        case kActionPushDescriptor:
            break;
        case kActionPullDescriptor:
            spin_lock(&self->lock);
            for (int i = 0; i < LANE_PENDING_DESC_NUM; i++) {
                if (self->pending_descs[i]) {
                    handle_id_t id = attach_handle(current_task->universe,
                                                   self->pending_descs[i]);
                    self->pending_descs[i] = NULL;
                }
            }
            spin_unlock(&self->lock);
            break;
        default:
            break;
        }
    }
    self->recv_pos = recv_pos;

    return kErrNone;
}
