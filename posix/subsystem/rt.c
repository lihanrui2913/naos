#include <uapi/stub.h>
#include <mm/page_table_flags.h>
#include "heap.h"

#define POSIX_SUBSYSTEM_HEAP_SIZE (1 * 1024 * 1024)

extern void main(int lane);

void rt_main(int lane) {
    k_allocate_restrictions_t res = {
        .address_bits = 64,
    };
    handle_id_t memory_handle;
    kAllocateMemory(POSIX_SUBSYSTEM_HEAP_SIZE,
                    PT_FLAG_R | PT_FLAG_W | PT_FLAG_U, &res, &memory_handle);
    void *heap_start = NULL;
    kMapMemory(memory_handle, kThisSpace, NULL, POSIX_SUBSYSTEM_HEAP_SIZE, 0,
               &heap_start);
    heap_init(heap_start, POSIX_SUBSYSTEM_HEAP_SIZE);

    main(lane);

    kPanic("Should not enter this\n", 24);
}
