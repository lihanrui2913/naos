#pragma once

#include <uapi/kcall.h>
#include <uapi/stub.h>
#include "vma.h"

typedef struct vm_context {
    handle_id_t space_handle;
    vma_manager_t vma_mgr;
} vm_context_t;

typedef struct fs_context {
} fs_context_t;

typedef struct file_context {
} file_context_t;

typedef struct signal_context {
} signal_context_t;

typedef struct posix_timer_context {
} posix_timer_context_t;

typedef struct process {
    vm_context_t *vm_ctx;
    fs_context_t *fs_ctx;
    file_context_t *file_ctx;
    signal_context_t *signal_ctx;
    posix_timer_context_t *posix_timer_ctx;
} process_t;

process_t *process_new(const char *path);
