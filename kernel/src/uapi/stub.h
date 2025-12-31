#pragma once

#include <uapi/kcall.h>

#if defined(__x86_64__)
#include <uapi/stub-x86_64.h>
#endif

#define __syscall1(n, a) __syscall1(n, (long)(a))
#define __syscall2(n, a, b) __syscall2(n, (long)(a), (long)(b))
#define __syscall3(n, a, b, c) __syscall3(n, (long)(a), (long)(b), (long)(c))
#define __syscall4(n, a, b, c, d)                                              \
    __syscall4(n, (long)(a), (long)(b), (long)(c), (long)(d))
#define __syscall5(n, a, b, c, d, e)                                           \
    __syscall5(n, (long)(a), (long)(b), (long)(c), (long)(d), (long)(e))
#define __syscall6(n, a, b, c, d, e, f)                                        \
    __syscall6(n, (long)(a), (long)(b), (long)(c), (long)(d), (long)(e),       \
               (long)(f))
#define __syscall7(n, a, b, c, d, e, f, g)                                     \
    __syscall7(n, (long)(a), (long)(b), (long)(c), (long)(d), (long)(e),       \
               (long)(f), (long)(g))

#define __SYSCALL_NARGS_X(a, b, c, d, e, f, g, h, n, ...) n
#define __SYSCALL_NARGS(...)                                                   \
    __SYSCALL_NARGS_X(__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1, 0, )
#define __SYSCALL_CONCAT_X(a, b) a##b
#define __SYSCALL_CONCAT(a, b) __SYSCALL_CONCAT_X(a, b)
#define __SYSCALL_DISP(b, ...)                                                 \
    __SYSCALL_CONCAT(b, __SYSCALL_NARGS(__VA_ARGS__))(__VA_ARGS__)

#define syscall(...) __SYSCALL_DISP(__syscall, __VA_ARGS__)

static inline k_error_t kLog(enum kLogSeverity log_severity, const char *string,
                             size_t len) {
    return (k_error_t)syscall(kCallBase + kCallLog, log_severity, string, len);
}

static inline __attribute__((noreturn)) void kPanic(const char *string,
                                                    size_t length) {
    syscall(kCallBase + KCallPanic, string, length);
    __builtin_unreachable();
}

static inline k_error_t kNop() {
    return (k_error_t)syscall(kCallBase + kCallNop);
}

static inline k_error_t kGetRandomBytes(void *buf, size_t wanted_size,
                                        size_t *actual_size) {
    return (k_error_t)syscall(kCallBase + kCallGetRandomBytes, buf, wanted_size,
                              actual_size);
}

static inline k_error_t kGetClock(int64_t *clock) {
    return (k_error_t)syscall(kCallBase + kCallGetClock, clock);
}

static inline k_error_t kCreateUniverse(handle_id_t *out) {
    return (k_error_t)syscall(kCallBase + kCallCreateUniverse, out);
}

static inline k_error_t kTransferDescriptor(handle_id_t handle,
                                            handle_id_t universe_handle,
                                            enum kTransferDescriptorFlags dir,
                                            handle_id_t *out_handle) {
    return (k_error_t)syscall(kCallBase + kCallTransferDescriptor, handle,
                              universe_handle, dir, out_handle);
}

static inline k_error_t kGetDescriptorInfo(handle_id_t handle,
                                           k_descriptor_info_t *info) {
    return (k_error_t)syscall(kCallBase + kCallGetDescriptorInfo, handle, info);
}

static inline k_error_t kCloseDescriptor(handle_id_t universe_handle,
                                         handle_id_t handle) {
    return (k_error_t)syscall(kCallBase + kCallCloseDescriptor, universe_handle,
                              handle);
}

// Use kResizeMemory to close
static inline k_error_t kAllocateMemory(size_t size, uint32_t flags,
                                        const k_allocate_restrictions_t *res,
                                        handle_id_t *out) {
    return (k_error_t)syscall(kCallBase + kCallAllocateMemory, size, flags, res,
                              out);
}

static inline k_error_t kResizeMemory(handle_id_t handle, size_t new_size) {
    return (k_error_t)syscall(kCallBase + kCallResizeMemory, handle, new_size);
}

static inline k_error_t kGetMemoryInfo(handle_id_t handle, size_t *len,
                                       size_t *info) {
    return (k_error_t)syscall(kCallBase + kCallGetMemoryInfo, handle, len,
                              info);
}

static inline k_error_t kSetMemoryInfo(handle_id_t handle, size_t info) {
    return (k_error_t)syscall(kCallBase + kCallSetMemoryInfo, handle, info);
}

static inline k_error_t kMapMemory(handle_id_t memory_handle,
                                   handle_id_t space_id, void *pointer,
                                   size_t size, uint32_t flags,
                                   void **actual_pointer) {
    return (k_error_t)syscall(kCallBase + kCallMapMemory, memory_handle,
                              space_id, pointer, size, flags, actual_pointer);
}

static inline k_error_t kUnmapMemory(handle_id_t memory_handle,
                                     handle_id_t space_handle, void *pointer,
                                     size_t size) {
    return (k_error_t)syscall(kCallBase + kCallUnMapMemory, memory_handle,
                              space_handle, pointer, size);
}

static inline k_error_t kCreatePhysicalMemory(uintptr_t paddr, size_t size,
                                              size_t info, handle_id_t *out) {
    return (k_error_t)syscall(kCallBase + kCallCreatePhysicalMemory, paddr,
                              size, info, out);
}

static inline k_error_t kCreateSpace(handle_id_t *out) {
    return (k_error_t)syscall(kCallBase + kCallCreateSpace, out);
}

static inline k_error_t kCreateThread(handle_id_t universe, handle_id_t space,
                                      const k_create_thread_arg_t *arg,
                                      uint64_t flags,
                                      handle_id_t *thread_handle_out) {
    return (k_error_t)syscall(kCallBase + kCallCreateThread, universe, space,
                              arg, flags, thread_handle_out);
}

static inline k_error_t kQueryThreadStats(handle_id_t thread_handle,
                                          k_thread_stats_t *stats) {

    return (k_error_t)syscall(kCallBase + kCallQueryThreadStats, thread_handle,
                              stats);
}

static inline k_error_t kYield() {
    return (k_error_t)syscall(kCallBase + kCallYield);
}

static inline k_error_t kSetPriority(handle_id_t handle, int priority) {
    return (k_error_t)syscall(kCallBase + kCallSetPriority, handle, priority);
}

static inline k_error_t kKillThread(handle_id_t handle) {
    return (k_error_t)syscall(kCallBase + kCallKillThread, handle);
}

static inline k_error_t kInterruptThread(handle_id_t handle) {
    return (k_error_t)syscall(kCallBase + kCallInterruptThread, handle);
}

static inline k_error_t kLoadRegisters(handle_id_t handle, int set,
                                       void *image) {
    return (k_error_t)syscall(kCallBase + kCallLoadRegisters, handle, set,
                              image);
}

static inline k_error_t kStoreRegisters(handle_id_t handle, int set,
                                        const void *image) {
    return (k_error_t)syscall(kCallBase + kCallStoreRegisters, handle, set,
                              image);
}

static inline k_error_t kWriteFsBase(void *pointer) {
    return (k_error_t)syscall(kCallBase + kCallWriteFsBase, pointer);
};

static inline k_error_t kReadFsBase(void **pointer) {
    return (k_error_t)syscall(kCallBase + kCallReadFsBase, pointer);
};

static inline k_error_t kWriteGsBase(void *pointer) {
    return (k_error_t)syscall(kCallBase + kCallWriteGsBase, pointer);
};

static inline k_error_t kReadGsBase(void **pointer) {
    return (k_error_t)syscall(kCallBase + kCallReadGsBase, pointer);
};

static inline k_error_t kFutexWait(int *pointer, int except, int64_t deadline) {
    return (k_error_t)syscall(kCallBase + kCallFutexWait, pointer, except,
                              deadline);
}

static inline k_error_t kFutexWake(int *pointer, int count) {
    return (k_error_t)syscall(kCallBase + kCallFutexWake, pointer, count);
}

static inline k_error_t kCreateStream(handle_id_t *handle_out1,
                                      handle_id_t *handle_out2) {
    return (k_error_t)syscall(kCallBase + kCallCreateStream, handle_out1,
                              handle_out2);
}

static inline k_error_t kSubmitDescriptor(handle_id_t lane_handle_id,
                                          k_action_t *action, size_t count,
                                          uint32_t flags) {
    return (k_error_t)syscall(kCallBase + kCallSubmitDescriptor, lane_handle_id,
                              action, count, flags);
}

static inline k_error_t kLookupInitramfs(const char *path,
                                         handle_id_t *ret_handle) {
    return (k_error_t)syscall(kCallBase + kCallLookupInitramfs, path,
                              ret_handle);
}

static inline k_error_t kReadInitramfs(handle_id_t handle, size_t offset,
                                       void *buf, size_t limit) {
    return (k_error_t)syscall(kCallBase + kCallReadInitramfs, handle, offset,
                              buf, limit);
}
