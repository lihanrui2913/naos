#include <arch/arch.h>
#include <arch/x64/syscall/nr.h>
#include <libs/strerror.h>
#include <kcall/kcall.h>

void syscall_init() {
    uint64_t efer;

    // 1. 启用 EFER.SCE (System Call Extensions)
    efer = rdmsr(MSR_EFER);
    efer |= 1; // 设置 SCE 位
    wrmsr(MSR_EFER, efer);

    uint16_t cs_sysret_cmp = SELECTOR_USER_CS - 16;
    uint16_t ss_sysret_cmp = SELECTOR_USER_DS - 8;
    uint16_t cs_syscall_cmp = SELECTOR_KERNEL_CS;
    uint16_t ss_syscall_cmp = SELECTOR_KERNEL_DS - 8;

    if (cs_sysret_cmp != ss_sysret_cmp) {
        printk("Sysret offset is not valid (1)");
        return;
    }

    if (cs_syscall_cmp != ss_syscall_cmp) {
        printk("Syscall offset is not valid (2)");
        return;
    }

    // 2. 设置 STAR MSR
    uint64_t star = 0;
    star = ((uint64_t)(SELECTOR_USER_DS - 8) << 48) | // SYSRET 的基础 CS
           ((uint64_t)SELECTOR_KERNEL_CS << 32);      // SYSCALL 的 CS
    wrmsr(MSR_STAR, star);

    // 3. 设置 LSTAR MSR (系统调用入口点)
    wrmsr(MSR_LSTAR, (uint64_t)syscall_exception);

    // 4. 设置 SYSCALL_MASK MSR (RFLAGS 掩码)
    wrmsr(MSR_SYSCALL_MASK, (1 << 9));
}

syscall_handle_t syscall_handlers[MAX_SYSCALL_NUM];
syscall_handle_t kcall_handlers[100];

void syscall_handler_init() {
    memset(syscall_handlers, 0, sizeof(syscall_handlers));

    memset(kcall_handlers, 0, sizeof(kcall_handlers));
    kcall_handlers[kCallLog] = (syscall_handle_t)kCallLogImpl;
    kcall_handlers[KCallPanic] = (syscall_handle_t)kCallPanicImpl;
    kcall_handlers[kCallNop] = (syscall_handle_t)kCallNopImpl;
    kcall_handlers[kCallGetRandomBytes] =
        (syscall_handle_t)kCallGetRandomBytesImpl;
    kcall_handlers[kCallGetClock] = (syscall_handle_t)kCallGetClockImpl;
    kcall_handlers[kCallCreateUniverse] = (syscall_handle_t)kCreateUniverseImpl;
    kcall_handlers[kCallGetDescriptorInfo] =
        (syscall_handle_t)kGetDescriptorInfoImpl;
    kcall_handlers[kCallCloseDescriptor] =
        (syscall_handle_t)kCloseDescriptorImpl;
    kcall_handlers[kCallAllocateMemory] = (syscall_handle_t)kAllocateMemoryImpl;
    kcall_handlers[kCallResizeMemory] = (syscall_handle_t)kResizeMemoryImpl;
    kcall_handlers[kCallGetMemoryInfo] = (syscall_handle_t)kGetMemoryInfoImpl;
    kcall_handlers[kCallSetMemoryInfo] = (syscall_handle_t)kSetMemoryInfoImpl;
    kcall_handlers[kCallMapMemory] = (syscall_handle_t)kMapMemoryImpl;
    kcall_handlers[kCallCreatePhysicalMemory] =
        (syscall_handle_t)kCreatePhysicalMemoryImpl;
    kcall_handlers[kCallCreateThread] = (syscall_handle_t)kCreateThreadImpl;
}

spinlock_t syscall_debug_lock = SPIN_INIT;

void syscall_handler(struct pt_regs *regs, uint64_t user_rsp) {
    regs->rip = regs->rcx;
    regs->rflags = regs->r11;
    regs->cs = SELECTOR_USER_CS;
    regs->ss = SELECTOR_USER_DS;
    regs->rsp = user_rsp;

    uint64_t idx = regs->rax & 0xFFFFFFFF;

    uint64_t arg1 = regs->rdi;
    uint64_t arg2 = regs->rsi;
    uint64_t arg3 = regs->rdx;
    uint64_t arg4 = regs->r10;
    uint64_t arg5 = regs->r8;
    uint64_t arg6 = regs->r9;

    if (idx >= kCallBase) {
        goto maybe_kcall;
    }

    if (idx > MAX_SYSCALL_NUM) {
        regs->rax = (uint64_t)-ENOSYS;
    }

    syscall_handle_t handler = syscall_handlers[idx];
    if (!handler) {
        regs->rax = (uint64_t)-ENOSYS;
    }

    regs->rax = 0;

    goto done;

maybe_kcall:
    syscall_handle_t kcall = kcall_handlers[idx - kCallBase];
    regs->rax = kcall(arg1, arg2, arg3, arg4, arg5, arg6);
done:
}
