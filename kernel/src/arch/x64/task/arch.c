#include <arch/x64/task/arch.h>
#include <task/task.h>

void kernel_thread_func();
asm("kernel_thread_func:\n\t"
    "    popq %r15\n\t"
    "    popq %r14\n\t"
    "    popq %r13\n\t"
    "    popq %r12\n\t"
    "    popq %r11\n\t"
    "    popq %r10\n\t"
    "    popq %r9\n\t"
    "    popq %r8\n\t"
    "    popq %rbx\n\t"
    "    popq %rcx\n\t"
    "    popq %rdx\n\t"
    "    popq %rsi\n\t"
    "    popq %rdi\n\t"
    "    popq %rbp\n\t"
    "    popq %rax\n\t"
    "    addq $0x38, %rsp\n\t"
    "    movq %rdx, %rdi\n\t"
    "    callq *%rbx\n\t"
    "    movq $0, %rdi\n\t"
    "    callq task_exit\n\t");

void task_arch_init(struct task *task, uint64_t stack, uint64_t entry,
                    uint64_t arg) {
    task->arch = malloc(sizeof(task_arch_info_t));
    memset(task->arch, 0, sizeof(task_arch_info_t));
    task->arch->ctx = (struct pt_regs *)stack - 1;
    memset(task->arch->ctx, 0, sizeof(struct pt_regs));
    task->arch->ctx->rbx = entry;
    task->arch->ctx->rdx = arg;
    task->arch->fpu_ctx = alloc_frames_bytes(sizeof(fpu_context_t));
    task->arch->fsbase = 0;
    task->arch->gsbase = 0;
    task->arch->rip = (uint64_t)kernel_thread_func;
    task->arch->rsp = (uint64_t)task->arch->ctx;
    task->arch->rbp = (uint64_t)task->arch->ctx;
}

extern void ret_from_intr();

void task_arch_init_user(struct task *task, uint64_t stack, uint64_t entry,
                         uint64_t usp) {

    task->arch = malloc(sizeof(task_arch_info_t));
    memset(task->arch, 0, sizeof(task_arch_info_t));
    task->arch->ctx = (struct pt_regs *)stack - 1;
    memset(task->arch->ctx, 0, sizeof(struct pt_regs));
    task->arch->ctx->cs = SELECTOR_USER_CS;
    task->arch->ctx->ss = SELECTOR_USER_DS;
    task->arch->ctx->rip = entry;
    task->arch->ctx->rsp = usp;
    task->arch->fpu_ctx = alloc_frames_bytes(sizeof(fpu_context_t));
    task->arch->fsbase = 0;
    task->arch->gsbase = 0;
    task->arch->rip = (uint64_t)ret_from_intr;
    task->arch->rsp = (uint64_t)task->arch->ctx;
    task->arch->rbp = (uint64_t)task->arch->ctx;
}

extern tss_t tss[MAX_CPU_NUM];

void __switch_to(task_t *prev, task_t *next) {
    prev->arch->fsbase = read_fsbase();
    prev->arch->gsbase = read_gsbase();

    if (prev->arch->fpu_ctx) {
        asm volatile("fxsave (%0)" ::"r"(prev->arch->fpu_ctx));
    }

    if (next->arch->fpu_ctx) {
        asm volatile("fxrstor (%0)" ::"r"(next->arch->fpu_ctx));
    }

    if (prev->mm != next->mm) {
        asm volatile("movq %0, %%cr3" ::"r"(next->mm->page_table_addr)
                     : "memory");
    }

    tss[current_cpu_id].rsp0 = next->kernel_stack;

    write_fsbase(next->arch->fsbase);
    write_gsbase(next->arch->gsbase);
}
