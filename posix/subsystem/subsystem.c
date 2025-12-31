#include "vfs.h"
#include "process.h"
#include <uapi/aether/protocols.h>

void posix_main(int lane) {
    receiver_t *request_receiver = create_receiver(lane);
    sender_t *response_sender = create_sender(lane);
    while (1) {
        response_t server_res;
        server_res.magic = RESPONSE_MAGIC;
        request_t client_req;
        receiver_recv_request(request_receiver, &client_req);
        k_error_t err = receiver_recv(request_receiver);
        if (err == kErrDismissed) {
            kCloseDescriptor(kThisUniverse, lane);
            kKillThread(kThisThread);
        }
        if (client_req.magic != REQUEST_MAGIC) {
            server_res.res_code = RESPONSE_CODE_FAIL;
            sender_send_response(response_sender, &server_res);
            sender_send(response_sender);
            continue;
        }
    }
}

#define USER_STACK_SIZE 65536

void main(int lane) {
    kLog(kLogSeverityInfo, "posix-subsystem is starting...\n", 33);

    vfs_init();

    spawn_init_process();

    while (1) {
        k_action_t accept_action = {
            .type = kActionAccept,
            .handle = -1,
        };
        k_error_t error = kSubmitDescriptor(lane, &accept_action, 1, 0);
        if (error == kErrNone) {
            handle_id_t universe_handle;
            kCreateUniverse(&universe_handle);
            handle_id_t remote_handle;
            kTransferDescriptor(accept_action.handle, universe_handle,
                                kTransferDescriptorOut, &remote_handle);

            k_allocate_restrictions_t res = {
                .address_bits = 64,
            };
            handle_id_t stack_memory_handle;
            kAllocateMemory(USER_STACK_SIZE, PT_FLAG_R | PT_FLAG_W | PT_FLAG_U,
                            &res, &stack_memory_handle);
            void *stack_start = NULL;
            kMapMemory(stack_memory_handle, kThisSpace, NULL, USER_STACK_SIZE,
                       0, &stack_start);

            handle_id_t service_handle;
            k_create_thread_arg_t arg;
            arg.ip = posix_main;
            arg.sp = stack_start + USER_STACK_SIZE;
            arg.arg = remote_handle;
            kCreateThread(universe_handle, kThisSpace, &arg, 0,
                          &service_handle);

            kUnmapMemory(stack_memory_handle, kThisSpace, stack_start,
                         USER_STACK_SIZE);
        }
    }
}
