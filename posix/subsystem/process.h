#pragma once

#include <libs/llist.h>
#include <mm/page_table_flags.h>
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
    struct llist_header node;
    handle_id_t thread_handle;
    uint64_t thread_id;
    vm_context_t *vm_ctx;
    fs_context_t *fs_ctx;
    file_context_t *file_ctx;
    signal_context_t *signal_ctx;
    posix_timer_context_t *posix_timer_ctx;
} process_t;

typedef struct posix_process_arg {
    handle_id_t space_handle;
    uint64_t load_start;
    uint64_t load_end;
    void *ip;
    int argc;
    char **argv;
    int envc;
    char **envp;
} posix_process_arg_t;

posix_process_arg_t *process_arg_new(char *path, char *argv[], char *envp[]);
void process_arg_free(posix_process_arg_t *arg);

process_t *process_find(uint64_t thread_id);

process_t *process_new(const posix_process_arg_t *arg);

process_t *spawn_init_process();

void process_init();
