#include <kcall/kcall.h>
#include <kcall/handle.h>
#include <task/universe.h>
#include <task/task.h>
#include <task/sched.h>

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
    detatch_handle(universe, handle_id);
    free(handle);
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
    handle->handle_type = MEMORY;
    if (r.address_bits <= 32) {
        handle->memory.address = alloc_frames_dma32(pages);
    } else {
        handle->memory.address = alloc_frames(pages);
    }
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

k_error_t kMapMemoryImpl(handle_id_t memory_handle, void *pointer,
                         uintptr_t offset, size_t size, uint32_t flags,
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
    if (!write_user_memory(actual_pointer, &pointer, sizeof(void *))) {
        return kErrFault;
    }
    map_page_range(get_current_page_dir(false), (uint64_t)pointer,
                   handle->memory.address, size, handle->memory.info);
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

k_error_t kCreateThreadImpl(handle_id_t universe_id, void *ip, void *sp,
                            uint32_t flags, handle_id_t *thread_handle_out) {
    universe_t *universe = get_universe(universe_id);
    if (!universe) {
        return kErrBadDescriptor;
    }

    handle_t *handle = malloc(sizeof(handle_t));
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
    task_t *task = task_create_user(universe, ip, sp, flags);
    handle->thread.task = task;

    return kErrNone;
}
