#include "heap.h"
#include "vfs.h"
#include "process.h"
#include "memory.h"
#include <uapi/aether/protocols.h>

void posix_main(int lane) {
    receiver_t *request_receiver = create_receiver(lane);
    sender_t *response_sender = create_sender(lane);
    while (1) {
        response_t *server_res = malloc(sizeof(response_t));
        memset(server_res, 0, sizeof(response_t));
        server_res->magic = RESPONSE_MAGIC;
        request_t client_req;
        receiver_recv_request(request_receiver, &client_req);
        k_error_t err = receiver_recv(request_receiver);
        if (err == kErrDismissed) {
            free(request_receiver);
            free(response_sender);
            kCloseDescriptor(kThisUniverse, lane);
            kKillThread(kThisThread);
            // Should not be here
        }

        if (client_req.magic != REQUEST_MAGIC) {
            server_res->res_code = -EINVAL;
            sender_send_response(response_sender, server_res);
            sender_send(response_sender);
            free(server_res);
            continue;
        }
        if (client_req.type != REQUEST_TYPE_POSIX) {
            server_res->res_code = -EINVAL;
            sender_send_response(response_sender, server_res);
            sender_send(response_sender);
            free(server_res);
            continue;
        }

        void *request_data = malloc(client_req.data_len);
        receiver_recv_data(request_receiver, request_data, client_req.data_len);
        receiver_recv(request_receiver);

        server_res->type = client_req.type;

        switch (client_req.opcode) {
        case REQUEST_VM_MAP:
            map_request_t *req = request_data;
            server_res->res_code =
                posix_vm_map(req->address_hint, req->size, req->mode,
                             req->flags, req->fd, req->rel_offset);
            break;

        default:
            server_res->res_code = -ENOSYS;
            break;
        }

        free(request_data);

        sender_send_response(response_sender, server_res);
        sender_send(response_sender);

        free(server_res);
    }
}

__attribute__((naked)) void _posix_main() { asm volatile("call posix_main"); }

#define USER_STACK_SIZE 131072

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

            handle_id_t space_handle;
            kCreateSpace(&space_handle);
            kForkMemory(kThisSpace, space_handle);
            k_allocate_restrictions_t res = {
                .address_bits = 64,
            };
            handle_id_t stack_memory_handle;
            kAllocateMemory(USER_STACK_SIZE, PT_FLAG_R | PT_FLAG_W | PT_FLAG_U,
                            &res, &stack_memory_handle);
            void *stack_start = NULL;
            kMapMemory(stack_memory_handle, space_handle, NULL, USER_STACK_SIZE,
                       0, &stack_start);

            handle_id_t service_handle;
            k_create_thread_arg_t arg;
            arg.ip = _posix_main;
            arg.sp = stack_start + USER_STACK_SIZE;
            arg.arg = remote_handle;
            kCreateThread(universe_handle, space_handle, &arg, 0,
                          &service_handle);
        }
    }
}
