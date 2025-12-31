#include <arch/arch.h>
#include <arch/x64/syscall/nr.h>
#include <libs/strerror.h>
#include <kcall/handle.h>
#include <kcall/kcall.h>
#include <task/task.h>

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
syscall_handle_t kcall_handlers[110];

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
    kcall_handlers[kCallTransferDescriptor] =
        (syscall_handle_t)kTransferDescriptorImpl;
    kcall_handlers[kCallGetDescriptorInfo] =
        (syscall_handle_t)kGetDescriptorInfoImpl;
    kcall_handlers[kCallCloseDescriptor] =
        (syscall_handle_t)kCloseDescriptorImpl;
    kcall_handlers[kCallFutexWait] = (syscall_handle_t)kFutexWaitImpl;
    kcall_handlers[kCallFutexWake] = (syscall_handle_t)kFutexWakeImpl;
    kcall_handlers[kCallAllocateMemory] = (syscall_handle_t)kAllocateMemoryImpl;
    kcall_handlers[kCallResizeMemory] = (syscall_handle_t)kResizeMemoryImpl;
    kcall_handlers[kCallGetMemoryInfo] = (syscall_handle_t)kGetMemoryInfoImpl;
    kcall_handlers[kCallSetMemoryInfo] = (syscall_handle_t)kSetMemoryInfoImpl;
    kcall_handlers[kCallMapMemory] = (syscall_handle_t)kMapMemoryImpl;
    kcall_handlers[kCallUnMapMemory] = (syscall_handle_t)kUnmapMemoryImpl;
    kcall_handlers[kCallCreatePhysicalMemory] =
        (syscall_handle_t)kCreatePhysicalMemoryImpl;
    kcall_handlers[kCallCreateStream] = (syscall_handle_t)kCreateStreamImpl;
    kcall_handlers[kCallCreateSpace] = (syscall_handle_t)kCreateSpaceImpl;
    kcall_handlers[kCallCreateThread] = (syscall_handle_t)kCreateThreadImpl;
    kcall_handlers[kCallSubmitDescriptor] =
        (syscall_handle_t)kSubmitDescriptorImpl;
    kcall_handlers[kCallLookupInitramfs] =
        (syscall_handle_t)kLookupInitramfsImpl;
    kcall_handlers[kCallReadInitramfs] = (syscall_handle_t)kReadInitramfsImpl;
}

spinlock_t syscall_debug_lock = SPIN_INIT;

extern handle_t *posix_lane;

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

    if (!current_task->posix_lane) {
        handle_id_t handle1, handle2;
        kCreateStreamImpl(&handle1, &handle2);
        current_task->posix_lane = current_task->universe->handles[handle1];
        spin_lock(&posix_lane->lane.lane->peer->lock);
        while (!posix_lane || !posix_lane->lane.lane->peer)
            schedule(SCHED_YIELD);
        posix_lane->lane.lane->peer->connections[0] =
            current_task->universe->handles[handle2]->lane.lane;
        spin_unlock(&posix_lane->lane.lane->peer->lock);
    }

    if (idx > MAX_SYSCALL_NUM) {
        printk("Syscall %d not implemented\n", idx);
        regs->rax = (uint64_t)-ENOSYS;
    }

    syscall_handle_t handler = syscall_handlers[idx];
    if (!handler) {
        printk("Syscall %d not implemented\n", idx);
        regs->rax = (uint64_t)-ENOSYS;
    }

    regs->rax = handler(arg1, arg2, arg3, arg4, arg5, arg6);

    goto done;

maybe_kcall:
    if (current_task->flags & K_THREAD_FLAGS_POSIX) {
        regs->rax = (uint64_t)-1;
        goto done;
    }

    syscall_handle_t kcall = kcall_handlers[idx - kCallBase];
    regs->rax = kcall(arg1, arg2, arg3, arg4, arg5, arg6);
done:
}
