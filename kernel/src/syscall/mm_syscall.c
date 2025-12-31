#include <syscall/syscall.h>
#include <syscall/mm_syscall.h>
#include <mm/mm.h>
#include <task/task.h>

uint64_t sys_mmap(uint64_t addr, uint64_t len, uint64_t prot, uint64_t flags,
                  uint64_t fd, uint64_t offset) {
    request_t *mmap_request = malloc(sizeof(request_t) + sizeof(map_request_t));
    mmap_request->magic = REQUEST_MAGIC;
    mmap_request->type = REQUEST_TYPE_POSIX;
    mmap_request->opcode = REQUEST_VM_MAP;
    mmap_request->pid = current_task->pid;
    mmap_request->data_len = sizeof(map_request_t);
    map_request_t *map_request = (map_request_t *)mmap_request->data;
    map_request->address_hint = addr;
    map_request->size = len;
    map_request->mode = prot;
    map_request->flags = flags;
    map_request->fd = fd;
    map_request->rel_offset = offset;
    send_request(mmap_request);
    free(mmap_request);
    response_t *mmap_response = recv_response();
    uint64_t ret = mmap_response->res_code;
    free(mmap_response);
    return ret;
}
