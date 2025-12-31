#include "memory.h"

#define USER_MMAP_START 0x0000100000000000
#define USER_MMAP_END 0x0000600000000000

uint64_t do_munmap(process_t *proc, uint64_t addr, uint64_t size) {
    addr = addr & (~(DEFAULT_PAGE_SIZE - 1));
    size = (size + DEFAULT_PAGE_SIZE - 1) & ~(DEFAULT_PAGE_SIZE - 1);

    vma_manager_t *mgr = &proc->vm_ctx->vma_mgr;
    if (!vma_find_intersection(mgr, addr, addr + size)) {
        return -EINVAL;
    }

    vma_t *vma = mgr->vma_list;
    vma_t *next;

    uint64_t start = addr;
    uint64_t end = addr + size;

    while (vma) {
        next = vma->vm_next;

        if (vma->vm_start >= start && vma->vm_end <= end) {
            vma_remove(mgr, vma);
            vma_free(vma);
        } else if (!(vma->vm_end <= start || vma->vm_start >= end)) {
            if (vma->vm_start < start && vma->vm_end > end) {
                vma_split(mgr, vma, end);
                vma_split(mgr, vma, start);
                vma_t *middle = vma->vm_next;
                vma_remove(mgr, middle);
                vma_free(middle);
            } else if (vma->vm_start < start) {
                vma->vm_end = start;
            } else if (vma->vm_end > end) {
                vma->vm_start = end;
                if (vma->vm_type == VMA_TYPE_FILE) {
                    vma->vm_offset += end - vma->vm_start;
                }
            }
        }

        vma = next;
    }

    // kUnmapMemory(vma->mhandle, proc->vm_ctx->space_handle, (void *)addr,
    // size);

    return 0;
}

static uint64_t find_unmapped_area(vma_manager_t *mgr, uint64_t hint,
                                   uint64_t len) {
    vma_t *vma;

    if (len == 0 || len > USER_MMAP_END - USER_MMAP_START) {
        return (uint64_t)-ENOMEM;
    }

    if (hint) {
        hint = PADDING_UP(hint, DEFAULT_PAGE_SIZE);
        if (hint >= USER_MMAP_START && hint <= USER_MMAP_END - len &&
            !vma_find_intersection(mgr, hint, hint + len)) {
            return hint;
        }
    }

    rb_node_t *node = rb_first(&mgr->vma_tree);

    if (!node) {
        return USER_MMAP_START + len <= USER_MMAP_END ? USER_MMAP_START
                                                      : (uint64_t)-ENOMEM;
    }

    vma = rb_entry(node, vma_t, vm_rb);

    if (vma->vm_start >= USER_MMAP_START + len) {
        return USER_MMAP_START;
    }

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

uint64_t posix_vm_map(process_t *proc, uint64_t addr, uint64_t len,
                      uint64_t prot, uint64_t flags, uint64_t fd,
                      uint64_t offset) {
    handle_id_t space_handle = proc->vm_ctx->space_handle;
    addr = PADDING_DOWN(addr, DEFAULT_PAGE_SIZE);
    len = PADDING_UP(len, DEFAULT_PAGE_SIZE);
    if (!len)
        return (uint64_t)-EINVAL;

    vma_manager_t *manager = &proc->vm_ctx->vma_mgr;

    spin_lock_no_irqsave(&manager->lock);

    uint64_t start_addr;
    if (flags & MAP_FIXED) {
        if (!addr) {
            spin_unlock_no_irqstore(&manager->lock);
            return (uint64_t)-EINVAL;
        }

        start_addr = (uint64_t)addr;

        vma_t *region =
            vma_find_intersection(manager, start_addr, start_addr + len);
        if (region) {
            if (flags & MAP_FIXED_NOREPLACE)
                return (uint64_t)-EEXIST;
            do_munmap(proc, start_addr, len);
        }
    } else {
        start_addr = find_unmapped_area(manager, addr, len);
        if (start_addr > (uint64_t)-4095UL)
            return start_addr;
    }

    vma_t *vma = vma_alloc();
    if (!vma) {
        spin_unlock_no_irqstore(&manager->lock);
        return (uint64_t)-ENOMEM;
    }

    vma->vm_start = start_addr;
    vma->vm_end = start_addr + len;
    vma->vm_flags = 0;

    if (prot & PROT_READ)
        vma->vm_flags |= VMA_READ;
    if (prot & PROT_WRITE)
        vma->vm_flags |= VMA_WRITE;
    if (prot & PROT_EXEC)
        vma->vm_flags |= VMA_EXEC;
    if (flags & MAP_SHARED)
        vma->vm_flags |= VMA_SHARED;

    handle_id_t mhandle;
    k_allocate_restrictions_t res = {.address_bits = 64};
    kAllocateMemory(len, PT_FLAG_R | PT_FLAG_W | PT_FLAG_U, &res, &mhandle);

    if (flags & MAP_ANONYMOUS) {
        vma->vm_type = VMA_TYPE_ANON;
        vma->vm_name = NULL;
        vma->vm_flags |= VMA_ANON;
    } else {
        vma->vm_type = VMA_TYPE_FILE;
        vma->vm_name = NULL;
        vma->vm_offset = offset;
    }

    if (vma_insert(manager, vma) != 0) {
        vma_free(vma);
        spin_unlock_no_irqstore(&manager->lock);
        return (uint64_t)-ENOMEM;
    }

    spin_unlock_no_irqstore(&manager->lock);

    void *out_pointer;
    kMapMemory(mhandle, space_handle, (void *)start_addr, len, 0, &out_pointer);
    void *this_space_out_pointer;
    kMapMemory(mhandle, kThisSpace, NULL, len, 0, &this_space_out_pointer);
    memset(this_space_out_pointer, 0, len);
    kUnmapMemory(mhandle, kThisSpace, this_space_out_pointer, len);

    return (uint64_t)out_pointer;
}
