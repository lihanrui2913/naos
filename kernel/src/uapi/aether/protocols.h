#pragma once

#include <libs/klibc.h>
#include <uapi/kcall.h>

typedef struct request {
    uint64_t magic;
    uint64_t type;
    uint64_t data_len;
    char data[];
} request_t;

typedef struct response {
    uint64_t magic;
    uint64_t type;
    uint64_t res_code;
    uint64_t data_len;
    char data[];
} response_t;

#define MAX_SEND_ONCE 64

typedef struct sender {
    k_action_t *actions;
} sender_t;

static inline sender_t *create_sender() {
    sender_t *s = calloc(1, sizeof(sender_t));
    s->actions = calloc(MAX_SEND_ONCE, sizeof(k_action_t));
    return s;
}

typedef struct receiver {
    k_action_t *actions;
} receiver_t;

static inline receiver_t *create_receiver() {
    receiver_t *r = calloc(1, sizeof(receiver_t));
    r->actions = calloc(MAX_SEND_ONCE, sizeof(k_action_t));
    return r;
}
