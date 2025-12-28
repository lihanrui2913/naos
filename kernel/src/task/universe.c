#include <task/universe.h>
#include <uapi/kcall.h>
#include <kcall/handle.h>

universe_t *create_universe() {
    universe_t *u = malloc(sizeof(universe_t));
    memset(u, 0, sizeof(universe_t));
    u->refcount = 1;
    u->max_handle_count = 128;
    u->handles = malloc(sizeof(handle_t *) * u->max_handle_count);
    memset(u->handles, 0, sizeof(handle_t *) * u->max_handle_count);
    return u;
}

handle_id_t attach_handle(universe_t *u, handle_t *h) {
    for (handle_id_t id = 0; id < u->max_handle_count; id++) {
        if (u->handles[id])
            continue;
        u->handles[id] = h;
        return id;
    }
    return K_ERR_NO_SPACE_FOR_FILE;
}

void detatch_handle(universe_t *u, handle_id_t id) {
    if (u->handles[id]) {
        free(u->handles[id]);
        u->handles[id] = NULL;
    }
}

void drop_universe(universe_t *u) {
    u->refcount--;
    if (u->refcount <= 0) {
        if (u->handles)
            free(u->handles);
        free(u);
    }
}
