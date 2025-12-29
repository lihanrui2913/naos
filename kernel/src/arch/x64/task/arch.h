#pragma once

#include <libs/klibc.h>
#include <arch/x64/irq/ptrace.h>

typedef struct fpu_context {
    uint16_t fcw;
    uint16_t fsw;
    uint16_t ftw;
    uint16_t fop;
    uint64_t word2;
    uint64_t word3;
    uint32_t mxscr;
    uint32_t mxcsr_mask;
    uint64_t mm[16];
    uint64_t xmm[32];
    uint64_t rest[12];
} fpu_context_t;

typedef struct task_arch_info {
    uint64_t rip;
    uint64_t rsp;
    uint64_t rbp;
    uint64_t fsbase;
    uint64_t gsbase;
    struct pt_regs *ctx;
    fpu_context_t *fpu_ctx;
} task_arch_info_t;

#define switch_to(prev, next)                                                  \
    do {                                                                       \
        asm volatile("pushq %%rbp\n\t"                                         \
                     "movq %%rsp, %0\n\t"                                      \
                     "movq %2, %%rsp\n\t"                                      \
                     "leaq 1f(%%rip), %%rax\n\t"                               \
                     "movq %%rax, %1\n\t"                                      \
                     "pushq %3\n\t"                                            \
                     "jmp __switch_to\n\t"                                     \
                     "1:\n\t"                                                  \
                     "popq %%rbp\n\t"                                          \
                     : "=m"(prev->arch->rsp), "=m"(prev->arch->rip)            \
                     : "m"(next->arch->rsp), "m"(next->arch->rip), "D"(prev),  \
                       "S"(next)                                               \
                     : "rax");                                                 \
    } while (0)

struct task;
void task_arch_init(struct task *task, uint64_t stack, uint64_t entry,
                    uint64_t arg);
void task_arch_init_user(struct task *task, uint64_t stack, uint64_t entry,
                         uint64_t usp, uint64_t arg);
