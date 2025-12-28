#pragma once

#include <libs/klibc.h>
#include <mm/mm.h>
#include <uapi/kernel.h>

typedef int64_t handle_id_t;

struct handle;
typedef struct handle handle_t;

typedef struct universe {
    handle_t **handles;
    uint64_t max_handle_count;
    int refcount;
} universe_t;

universe_t *create_universe();

handle_id_t attach_handle(universe_t *u, handle_t *h);
void detatch_handle(universe_t *u, handle_id_t id);

void drop_universe(universe_t *u);
