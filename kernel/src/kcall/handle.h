#pragma once

#include <libs/llist.h>
#include <task/universe.h>

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

typedef struct queue_node {
    struct llist_header node;
    int action_type;
    char *buffer;
    size_t length;
} queue_node_t;

typedef struct queue_handle {
    struct llist_header nodes;
} queue_handle_t;

#define LANE_BUFFER_SIZE 65536
#define LANE_PENDING_DESC_NUM 64

typedef struct lane {
    spinlock_t lock;
    handle_t **pending_descs;
    uint8_t *recv_buff;
    size_t recv_pos;
    size_t recv_size;

    struct lane *peer;
} lane_t;

typedef struct lane_handle {
    lane_t *lane;
} lane_handle_t;

typedef struct handle {
    enum {
        UNIVERSE,
        MEMORY,
        THREAD,
        QUEUE,
        LANE,
    } handle_type;
    int refcount;
    union {
        universe_handle_t universe;
        memory_handle_t memory;
        thread_handle_t thread;
        queue_handle_t queue;
        lane_handle_t lane;
    };
} handle_t;
