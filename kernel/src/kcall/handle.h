#pragma once

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

typedef struct handle {
    enum {
        UNIVERSE,
        MEMORY,
        THREAD,
    } handle_type;
    union {
        universe_handle_t universe;
        memory_handle_t memory;
        thread_handle_t thread;
    };
} handle_t;
