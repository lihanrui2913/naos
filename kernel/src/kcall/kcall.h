#pragma once

#include <uapi/kcall.h>

k_error_t kCallLogImpl(enum kLogSeverity severity, const char *string,
                       size_t length);
void kCallPanicImpl(const char *string, size_t length);
k_error_t kCallNopImpl();
k_error_t kCallGetRandomBytesImpl(void *buf, size_t wanted_size,
                                  size_t *actual_size);
k_error_t kCallGetClockImpl(int64_t *clock);
k_error_t kCreateUniverseImpl(handle_id_t *out);

k_error_t kGetDescriptorInfoImpl(handle_id_t handle, k_descriptor_info_t *info);
k_error_t kCloseDescriptorImpl(handle_id_t universe_handle, handle_id_t handle);
k_error_t kAllocateMemoryImpl(size_t size, uint32_t flags,
                              const k_allocate_restrictions_t *res,
                              handle_id_t *out);
k_error_t kResizeMemoryImpl(handle_id_t handle, size_t new_size);
k_error_t kGetMemoryInfoImpl(handle_id_t handle, size_t *len, size_t *info);
k_error_t kSetMemoryInfoImpl(handle_id_t handle, size_t info);
k_error_t kMapMemoryImpl(handle_id_t memory_handle, void *pointer,
                         uintptr_t offset, size_t size, uint32_t flags,
                         void **actual_pointer);
k_error_t kUnmapMemoryImpl(handle_id_t memory_handle, void *pointer,
                           size_t size);
k_error_t kCreatePhysicalMemoryImpl(uintptr_t paddr, size_t size, size_t info,
                                    handle_id_t *out);

k_error_t kCreateThreadImpl(handle_id_t universe, void *ip, void *sp,
                            uint32_t flags, handle_id_t *thread_handle_out);
