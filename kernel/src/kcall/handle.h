#pragma once

#include <libs/llist.h>
#include <task/universe.h>
#include <init/initramfs.h>

typedef struct universe_handle {
    universe_t *universe;
} universe_handle_t;

typedef struct memory_handle {
    uintptr_t address;
    size_t len;
    size_t info;
} memory_handle_t;

struct task;
typedef struct task task_t;

typedef struct thread_handle {
    task_t *task;
} thread_handle_t;

typedef struct space {
    uint64_t *page_table_addr;
} space_t;

typedef struct space_handle {
    space_t *space;
} space_handle_t;

typedef struct initramfs_desc_handle {
    initramfs_handle_t *handle;
} initramfs_desc_handle_t;

#define LANE_BUFFER_SIZE 65536
#define LANE_PENDING_DESC_NUM 64
#define LANE_MAX_CONNECTIONS 16

typedef struct lane {
    spinlock_t lock;
    handle_t **pending_descs;
    uint8_t *recv_buff;
    size_t recv_pos;
    size_t recv_size;

    struct lane *peer;
    struct lane *connections[LANE_MAX_CONNECTIONS];
    bool peer_connected;
} lane_t;

typedef struct lane_handle {
    lane_t *lane;
} lane_handle_t;

typedef struct handle {
    enum {
        UNIVERSE,
        MEMORY,
        THREAD,
        SPACE,
        INITRAMFS,
        LANE,
    } handle_type;
    int refcount;
    union {
        universe_handle_t universe;
        memory_handle_t memory;
        thread_handle_t thread;
        space_handle_t space;
        initramfs_desc_handle_t initramfs;
        lane_handle_t lane;
    };
} handle_t;
