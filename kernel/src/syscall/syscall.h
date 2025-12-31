#pragma once

#include <arch/arch.h>
#include <uapi/aether/protocols.h>
#include <syscall/mm_syscall.h>

void send_request(request_t *req);
response_t *recv_response(void);
