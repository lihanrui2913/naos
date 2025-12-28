#pragma once

#include <libs/klibc.h>
#include <task/universe.h>

enum {
    kCallBase = 0x80000000,

    kCallLog = 1,
    KCallPanic = 2,
    kCallNop = 3,

    kCallGetRandomBytes = 5,
    kCallGetClock = 6,

    kCallCreateUniverse = 10,
    kCallAllocateMemory = 11,
    kCallResizeMemory = 12,
    kCallAccessPhysicalMemory = 13,
    kCallMapMemory = 14,
    kCallUnMapMemory = 15,
    kCallGetMemoryInfo = 16,
    kCallSetMemoryInfo = 17,
    kCallForkMemory = 18,
    kCallCreatePhysicalMemory = 19,

    kCallTransferDescriptor = 20,
    kCallGetDescriptorInfo = 21,
    kCallCloseDescriptor = 22,

    kCallLoadRegisters = 30,
    kCallStoreRegisters = 31,

    kCallFutexWait = 40,
    kCallFutexWake = 41,

    kCallCreateThread = 50,
    kCallQueryThreadStats = 51,
    kCallYield = 52,
    kCallSetPriority = 53,
    kCallKillThread = 54,
    kCallInterruptThread = 55,

    kCallCreateStream = 60,

    kCallSubmitDescriptor = 70,

#if defined(__x86_64__)
    kCallReadFsBase = 100,
    kCallWriteFsBase,
    kCallReadGsBase,
    kCallWriteGsBase,
#endif
};

typedef enum k_error {
    kErrNone = 0,
    kErrBufferTooSmall = 1,
    kErrBadDescriptor = 2,
    kErrTimeout = 3,
    kErrNoDescriptor = 4,
    kErrIllegalSyscall = 5,
    kErrIllegalArgs = 7,
    kErrLaneShutdown = 8,
    kErrEndOfLane = 9,
    kErrFault = 10,
    kErrThreadTerminated = 11,
    kErrCancelled = 12,
    kErrTransmissionMismatch = 13,
    kErrQueueTooSmall = 14,
    kErrIllegalState = 15,
    kErrNoHardwareSupport = 16,
    kErrNoMemory = 17,
    kErrUnsupportedOperation = 18,
    kErrOutOfBounds = 19,
    kErrDismissed = 20,
    kErrRemoteFault = 21,
    kErrAlreadyExists = 22
} k_error_t;

enum kLogSeverity {
    kLogSeverityEmergency,
    kLogSeverityAlert,
    kLogSeverityCritical,
    kLogSeverityError,
    kLogSeverityWarning,
    kLogSeverityNotice,
    kLogSeverityInfo,
    kLogSeverityDebug,
};

enum kTransferDescriptorFlags {
    kTransferDescriptorOut,
    kTransferDescriptorIn,
};

typedef struct k_descriptor_info {
    int type;
} k_descriptor_info_t;

typedef struct k_allocate_restrictions {
    int address_bits;
} k_allocate_restrictions_t;

#define K_MEMORY_FLAGS_R ((size_t)1 << 0)
#define K_MEMORY_FLAGS_W ((size_t)1 << 1)
#define K_MEMORY_FLAGS_RW (K_MEMORY_FLAGS_R | K_MEMORY_FLAGS_W)
#define K_MEMORY_FLAGS_CACHE_WT ((size_t)1 << 2)
#define K_MEMORY_FLAGS_CACHE_WB ((size_t)1 << 3)

typedef struct k_thread_stats {
    uint64_t user_time;
} k_thread_stats_t;

enum {
    kNullHandle = 0,
    kThisUniverse = -1,
    kThisThread = -2,
};

enum {
    kActionDismiss,
    kActionOffer,
    kActionAccept,
    kActionSendFromBuffer,
    kActionRecvToBuffer,
    kActionPushDescriptor,
    kActionPullDescriptor,
};

typedef struct k_action {
    int type;
    uint32_t flags;
    void *buffer;
    size_t length;
    handle_id_t handle;
} k_action_t;

typedef struct k_queue_parameters {
    uint32_t flags;
    unsigned int ring_shift;
    unsigned int num_chunks;
    size_t chunk_size;
} k_queue_parameters_t;

#define KCALL_SUBMIT_NO_RECEIVING (1UL << 0)
