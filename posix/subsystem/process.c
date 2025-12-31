#include "heap.h"
#include "process.h"
#include "elf.h"

struct llist_header processes_list;

vm_context_t *vm_context_new(handle_id_t space_handle) {
    vm_context_t *ctx = malloc(sizeof(vm_context_t));
    ctx->space_handle = space_handle;
    memset(&ctx->vma_mgr, 0, sizeof(vma_manager_t));
    ctx->vma_mgr.initialized = true;
    return ctx;
}

posix_process_arg_t *process_arg_new(char *path, char *argv[], char *envp[]) {
    // TODO
    return NULL;
}

void process_arg_free(posix_process_arg_t *arg) {
    if (arg->argv) {
        for (int i = 0; i < arg->argc; i++) {
            free(arg->argv[i]);
        }
        free(arg->argv);
    }
    if (arg->envp) {
        for (int i = 0; i < arg->envc; i++) {
            free(arg->envp[i]);
        }
        free(arg->envp);
    }
}

#define PUSH_TO_STACK(a, b, c)                                                 \
    a -= sizeof(b);                                                            \
    *((b *)(a)) = c

#define PUSH_BYTES_TO_STACK(stack_ptr, data, len)                              \
    do {                                                                       \
        stack_ptr -= (len);                                                    \
        memcpy((void *)(stack_ptr), (data), (len));                            \
    } while (0)

#define ALIGN_STACK_DOWN(stack_ptr, alignment)                                 \
    stack_ptr = (stack_ptr) & ~((alignment) - 1)

void *push_infos(void *current_stack, char *argv[], int argv_count,
                 char *envp[], int envp_count, uint64_t e_entry, uint64_t phdr,
                 uint64_t phnum, uint64_t at_base) {
    void *tmp_stack = current_stack;

    uint64_t random_values[2];
    kGetClock((int64_t *)&random_values[0]);
    kGetClock((int64_t *)&random_values[1]);
    PUSH_BYTES_TO_STACK(tmp_stack, random_values, 16);
    uint64_t random_ptr = (uint64_t)tmp_stack;

    uint64_t *envp_addrs = NULL;
    if (envp_count > 0 && envp != NULL) {
        envp_addrs = (uint64_t *)malloc(envp_count * sizeof(uint64_t));

        for (int i = envp_count - 1; i >= 0; i--) {
            size_t len = strlen(envp[i]) + 1;
            PUSH_BYTES_TO_STACK(tmp_stack, envp[i], len);
            envp_addrs[i] = (uint64_t)tmp_stack;
        }
    }

    uint64_t *argv_addrs = NULL;
    if (argv_count > 0 && argv != NULL) {
        argv_addrs = (uint64_t *)malloc(argv_count * sizeof(uint64_t));

        // 从后向前推送
        for (int i = argv_count - 1; i >= 0; i--) {
            size_t len = strlen(argv[i]) + 1;
            PUSH_BYTES_TO_STACK(tmp_stack, argv[i], len);
            argv_addrs[i] = (uint64_t)tmp_stack;
        }
    }

    uint64_t total_len = sizeof(uint64_t) +
                         (argv_count + 1) * sizeof(uint64_t) +
                         (envp_count + 1) * sizeof(uint64_t);
    tmp_stack -= ((uintptr_t)tmp_stack - total_len) % 0x10;

    PUSH_TO_STACK(tmp_stack, uint64_t, 0);
    PUSH_TO_STACK(tmp_stack, uint64_t, AT_NULL);

    PUSH_TO_STACK(tmp_stack, uint64_t, DEFAULT_PAGE_SIZE);
    PUSH_TO_STACK(tmp_stack, uint64_t, AT_PAGESZ);

    PUSH_TO_STACK(tmp_stack, uint64_t, random_ptr);
    PUSH_TO_STACK(tmp_stack, uint64_t, AT_RANDOM);

    PUSH_TO_STACK(tmp_stack, uint64_t, at_base);
    PUSH_TO_STACK(tmp_stack, uint64_t, AT_BASE);

    PUSH_TO_STACK(tmp_stack, uint64_t, e_entry);
    PUSH_TO_STACK(tmp_stack, uint64_t, AT_ENTRY);

    PUSH_TO_STACK(tmp_stack, uint64_t, phnum);
    PUSH_TO_STACK(tmp_stack, uint64_t, AT_PHNUM);

    PUSH_TO_STACK(tmp_stack, uint64_t, sizeof(Elf64_Phdr));
    PUSH_TO_STACK(tmp_stack, uint64_t, AT_PHENT);

    PUSH_TO_STACK(tmp_stack, uint64_t, phdr);
    PUSH_TO_STACK(tmp_stack, uint64_t, AT_PHDR);

    // NULL 结束标记
    PUSH_TO_STACK(tmp_stack, uint64_t, 0);

    if (envp_count > 0 && envp_addrs != NULL) {
        for (int i = envp_count - 1; i >= 0; i--) {
            PUSH_TO_STACK(tmp_stack, uint64_t, envp_addrs[i]);
        }
    }

    // NULL 结束标记
    PUSH_TO_STACK(tmp_stack, uint64_t, 0);

    if (argv_count > 0 && argv_addrs != NULL) {
        for (int i = argv_count - 1; i >= 0; i--) {
            PUSH_TO_STACK(tmp_stack, uint64_t, argv_addrs[i]);
        }
    }

    PUSH_TO_STACK(tmp_stack, uint64_t, argv_count);

    if (argv_addrs)
        free(argv_addrs);
    if (envp_addrs)
        free(envp_addrs);

    return tmp_stack;
}

process_t *process_find(uint64_t thread_id) {
    process_t *proc, *tmp;
    llist_for_each(proc, tmp, &processes_list, node) {
        if (proc->thread_id == thread_id)
            return proc;
    }
    return NULL;
}

#define USER_STACK_START 0x00005fffff000000
#define USER_STACK_END 0x0000600000000000

process_t *process_new(const posix_process_arg_t *arg) {
    process_t *p = malloc(sizeof(process_t));
    memset(p, 0, sizeof(process_t));
    p->vm_ctx = vm_context_new(arg->space_handle);
    // Other
    handle_id_t stack_memory_handle;
    k_allocate_restrictions_t res = {.address_bits = 64};
    kAllocateMemory(USER_STACK_END - USER_STACK_START,
                    PT_FLAG_R | PT_FLAG_W | PT_FLAG_U, &res,
                    &stack_memory_handle);
    void *current_stack;
    kMapMemory(stack_memory_handle, arg->space_handle, (void *)USER_STACK_START,
               USER_STACK_END - USER_STACK_START, 0, &current_stack);
    current_stack += (USER_STACK_END - USER_STACK_START);
    void *this_space_current_stack;
    kMapMemory(stack_memory_handle, kThisSpace, NULL,
               USER_STACK_END - USER_STACK_START, 0, &this_space_current_stack);
    void *new_stack = push_infos(
        this_space_current_stack + (USER_STACK_END - USER_STACK_START),
        arg->argv, arg->argc, arg->envp, arg->envc, (uint64_t)arg->ip, 0, 0, 0);
    current_stack -= (this_space_current_stack +
                      (USER_STACK_END - USER_STACK_START) - new_stack);

    handle_id_t universe_handle;
    kCreateUniverse(&universe_handle);
    k_create_thread_arg_t kcarg;
    kcarg.ip = arg->ip;
    kcarg.sp = current_stack;
    kcarg.arg = 0;

    handle_id_t thread_handle;
    kCreateThread(universe_handle, arg->space_handle, &kcarg,
                  K_THREAD_FLAGS_POSIX, &thread_handle);
    p->thread_handle = thread_handle;
    k_thread_stats_t stats;
    kQueryThreadStats(p->thread_handle, &stats);
    p->thread_id = stats.thread_id;
    llist_init_head(&p->node);
    llist_append(&processes_list, &p->node);

    vma_t *exec_vma = vma_alloc();
    exec_vma->vm_start = arg->load_start;
    exec_vma->vm_end = arg->load_end;
    vma_insert(&p->vm_ctx->vma_mgr, exec_vma);
    vma_t *stack_vma = vma_alloc();
    stack_vma->vm_start = USER_STACK_START;
    stack_vma->vm_end = USER_STACK_END;
    vma_insert(&p->vm_ctx->vma_mgr, stack_vma);

    return p;
}

#define INIT_PROCESS "posix-init"

process_t *spawn_init_process() {
    handle_id_t initramfs_handle;
    kLookupInitramfs(INIT_PROCESS, &initramfs_handle);
    posix_process_arg_t *arg = malloc(sizeof(posix_process_arg_t));
    memset(arg, 0, sizeof(posix_process_arg_t));
    Elf64_Ehdr ehdr;
    kReadInitramfs(initramfs_handle, 0, &ehdr, sizeof(Elf64_Ehdr));
    Elf64_Phdr *phdrs = calloc(ehdr.e_phnum, sizeof(Elf64_Phdr));
    kReadInitramfs(initramfs_handle, ehdr.e_phoff, phdrs,
                   ehdr.e_phnum * sizeof(Elf64_Phdr));
    handle_id_t space_handle;
    kCreateSpace(&space_handle);
    uint64_t load_low = UINT64_MAX;
    uint64_t load_high = 0;
    for (Elf64_Half i = 0; i < ehdr.e_phnum; i++) {
        Elf64_Phdr *phdr = &phdrs[i];
        if (phdr->p_type != PT_LOAD)
            continue;
        if (phdr->p_vaddr < load_low) {
            load_low = phdr->p_vaddr;
        }
        if (phdr->p_vaddr + phdr->p_memsz > load_high) {
            load_high = phdr->p_vaddr + phdr->p_memsz;
        }
        size_t start = PADDING_DOWN(phdr->p_vaddr, DEFAULT_PAGE_SIZE);
        size_t offset = phdr->p_vaddr - start;
        size_t size =
            PADDING_UP(phdr->p_vaddr + phdr->p_memsz, DEFAULT_PAGE_SIZE) -
            start;
        k_allocate_restrictions_t res = {.address_bits = 64};
        handle_id_t memory_handle;
        kAllocateMemory(size, PT_FLAG_R | PT_FLAG_W | PT_FLAG_U, &res,
                        &memory_handle);
        void *out_space_pointer;
        kMapMemory(memory_handle, space_handle, (void *)start, size, 0,
                   &out_space_pointer);
        void *out_this_space_pointer;
        kMapMemory(memory_handle, kThisSpace, NULL, size, 0,
                   &out_this_space_pointer);
        kReadInitramfs(initramfs_handle, phdr->p_offset,
                       out_this_space_pointer + offset, phdr->p_filesz);
        if (phdr->p_memsz > phdr->p_filesz) {
            memset((void *)(out_this_space_pointer + offset + phdr->p_filesz),
                   0, phdr->p_memsz - phdr->p_filesz);
        }
        kUnmapMemory(memory_handle, kThisSpace, out_this_space_pointer, size);
    }
    kCloseDescriptor(kThisUniverse, initramfs_handle);
    arg->space_handle = space_handle;
    load_low = PADDING_DOWN(load_low, DEFAULT_PAGE_SIZE);
    load_high = PADDING_UP(load_high, DEFAULT_PAGE_SIZE);
    arg->load_start = load_low;
    arg->load_end = load_high;
    arg->ip = (void *)ehdr.e_entry;
    arg->argc = 1;
    arg->argv = calloc(1, sizeof(char *));
    arg->argv[0] = strdup(INIT_PROCESS);
    process_t *process = process_new(arg);
    process_arg_free(arg);
    return process;
}

void process_init() { llist_init_head(&processes_list); }
