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
k_error_t kTransferDescriptorImpl(handle_id_t handle,
                                  handle_id_t universe_handle,
                                  enum kTransferDescriptorFlags dir,
                                  handle_id_t *out_handle);
k_error_t kGetDescriptorInfoImpl(handle_id_t handle, k_descriptor_info_t *info);
k_error_t kCloseDescriptorImpl(handle_id_t universe_handle, handle_id_t handle);
k_error_t kFutexWaitImpl(int *pointer, int except, int64_t deadline);
k_error_t kFutexWakeImpl(int *pointer, int count);
k_error_t kAllocateMemoryImpl(size_t size, uint32_t flags,
                              const k_allocate_restrictions_t *res,
                              handle_id_t *out);
k_error_t kResizeMemoryImpl(handle_id_t handle, size_t new_size);
k_error_t kGetMemoryInfoImpl(handle_id_t handle, size_t *len, size_t *info);
k_error_t kSetMemoryInfoImpl(handle_id_t handle, size_t info);
k_error_t kMapMemoryImpl(handle_id_t memory_handle, handle_id_t space_id,
                         void *pointer, size_t size, uint32_t flags,
                         void **actual_pointer);
k_error_t kUnmapMemoryImpl(handle_id_t memory_handle, handle_id_t space_id,
                           void *pointer, size_t size);
k_error_t kCreatePhysicalMemoryImpl(uintptr_t paddr, size_t size, size_t info,
                                    handle_id_t *out);
k_error_t kForkMemoryImpl(handle_id_t source, handle_id_t dest);
k_error_t kCreateStreamImpl(handle_id_t *handle_out1, handle_id_t *handle_out2);
k_error_t kCreateSpaceImpl(handle_id_t *out);
k_error_t kCreateThreadImpl(handle_id_t universe, handle_id_t space,
                            const k_create_thread_arg_t *arg, uint64_t flags,
                            handle_id_t *thread_handle_out);
k_error_t kLoadRegistersImpl(handle_id_t handle, int set, void *image);
k_error_t kStoreRegistersImpl(handle_id_t handle, int set, const void *image);
k_error_t kSubmitDescriptorImpl(handle_id_t handle, k_action_t *action,
                                size_t count, uint32_t flags);
k_error_t kLookupInitramfsImpl(const char *path, handle_id_t *ret_handle);
k_error_t kReadInitramfsImpl(handle_id_t handle, size_t offset, void *buf,
                             size_t limit);
