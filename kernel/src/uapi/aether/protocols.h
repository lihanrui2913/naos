#pragma once

#include <libs/klibc.h>
#include <uapi/stub.h>

extern void *calloc(size_t num, size_t size);

#define REQUEST_TYPE_NONE 0

#define REQUEST_TYPE_POSIX 1
enum {
    REQUEST_CLONE,
    REQUEST_EXEC,
    REQUEST_WAIT,
    REQUEST_VM_MAP,
    REQUEST_VM_REMAP,
    REQUEST_VM_PROTECT,
    REQUEST_VM_UNMAP,
    REQUEST_MOUNT,
    REQUEST_CHROOT,
    REQUEST_CHDIR,
    REQUEST_FCHDIR,
    REQUEST_SYMLINKAT,
    REQUEST_READLINK,
    REQUEST_READ,
    REQUEST_WRITE,
    REQUEST_STAT,
    REQUEST_FSTAT,
    REQUEST_SEEK_ABS,
    REQUEST_SEEK_REL,
    REQUEST_SEEK_EOF,
    REQUEST_DUP,
    REQUEST_DUP2,
    REQUEST_TTY_NAME,
    REQUEST_GETCWD,
    REQUEST_FD_GET_FLAGS,
    REQUEST_FD_SET_FLAGS,
    REQUEST_GET_RESOURCE_USAGE,
    REQUEST_SETSID,
    REQUEST_SIGACTION,

    REQUEST_PIPE_CREATE,

    REQUEST_EPOLL_CALL,
    REQUEST_EPOLL_CREATE,
    REQUEST_EPOLL_ADD,
    REQUEST_EPOLL_MOD,
    REQUEST_EPOLL_DEL,
    REQUEST_EPOLL_WAIT,

    REQUEST_SIGNALFD_CREATE,

    REQUEST_HELFD_ATTACH,
    REQUEST_HELFD_CLONE,
};
enum {
    OPEN_MODE_REGULAR = 1,
    OPEN_MODE_HELFD = 2,
};
enum {
    FT_UNKNOWN = 0,
    FT_REGULAR = 1,
    FT_DIRECTORY = 2,
    FT_SYMLINK = 3,
    FT_FIFO = 4,
    FT_SOCKET = 5,
    FT_CHAR_DEVICE = 6,
    FT_BLOCK_DEVICE = 7
};
enum {
    OF_CREATE = 1,
    OF_EXCLUSIVE = 2,
    OF_NONBLOCK = 4,
    OF_CLOEXEC = 256,
    OF_RDONLY = 8,
    OF_WRONLY = 16,
    OF_RDWR = 32,
    OF_TRUNC = 64,
    OF_PATH = 128,
    OF_NOCTTY = 512,
    OF_APPEND = 1024,
    OF_NOFOLLOW = 2048,
    OF_DIRECTORY = 4096
};
enum { EFD_CLOEXEC = 1, EFD_NONBLOCK = 2, EFD_SEMAPHORE = 4 };

typedef struct map_request {
    int32_t fd;
    uint32_t mode;
    uint32_t flags;
    uint64_t address_hint;
    int64_t rel_offset;
    uint64_t size;
} map_request_t;

typedef struct open_at_request {
    int32_t fd;
    uint64_t open_flags;
    uint64_t open_mode;
    char path[0];
} open_at_request_t;

typedef struct close_request {
    int32_t fd;
} close_request_t;

typedef struct is_tty_request {
    int32_t fd;
} is_tty_request_t;

typedef struct rename_at_request {
    int32_t fd;
    int32_t newfd;
    int path_len;
    int target_path_len;
    char data[0];
} rename_at_request;

typedef struct getuid_request {
} getuid_request_t;

typedef struct setuid_request {
    uint64_t uid;
} setuid_request_t;

typedef struct geteuid_request {
} geteuid_request_t;

typedef struct seteuid_request {
    uint64_t euid;
} seteuid_request_t;

typedef struct getgid_request {
} getgid_request_t;

typedef struct setgid_request {
    uint64_t gid;
} setgid_request_t;

typedef struct getegid_request {
} getegid_request_t;

typedef struct setegid_request {
    uint64_t egid;
} setegid_request_t;

typedef struct unlinkat_request {
    int32_t fd;
    int32_t flags;
    char path[0];
} unlinkat_request_t;

typedef struct fstatat_request {
    int32_t fd;
    int32_t flags;
    char path[0];
} fstatat_request_t;

typedef struct mkfifoat_request {
    int32_t fd;
    int32_t mode;
    char path[0];
} mkfifoat_request_t;

typedef struct linkat_request {
    int32_t fd;
    int32_t newfd;
    int32_t flags;
    int path_len;
    int target_path_len;
    char data[0];
} linkat_request;

typedef struct utimesat_request {
    int32_t fd;
    int32_t flags;
    int32_t mode;
    uint64_t atime_sec;
    uint64_t atime_nsec;
    uint64_t mtime_sec;
    uint64_t mtime_nsec;
    char path[0];
} utimesat_request_t;

typedef struct rmdir_request {
    char path[0];
} rmdir_request_t;

typedef struct inotify_add_request {
    int32_t fd;
    int32_t flags;
    char path[0];
} inotify_add_request_t;

typedef struct inotify_create_request {
    int32_t flags;
} inotify_create_request_t;

typedef struct eventfd_create_request {
    uint32_t initval;
    uint64_t efd_flags;
} eventfd_create_request_t;

typedef struct socket_request {
    int32_t flags;
    int32_t domain;
    int32_t socktype;
    int32_t protocol;
} socket_request_t;

typedef struct socketpair_request {
    int32_t flags;
    int32_t domain;
    int32_t socktype;
    int32_t protocol;
} socketpair_request_t;

typedef struct accept_request {
    int32_t fd;
} accept_request_t;

typedef struct mount_request {
    int path_len;
    int target_path_len;
    int fstype_len;
    char data[0];
} mount_request_t;

typedef struct symlinkat_request {
    int32_t fd;
    int path_len;
    int target_path_len;
    char data[0];
} symlinkat_request_t;

typedef struct getppid_request {
} getppid_request_t;

typedef struct mknodat_request {
    int32_t dirfd;
    int32_t mode;
    int32_t device;
    char path[0];
} mknodat_request_t;

typedef struct setpgid_request {
    int64_t pid;
    int64_t pgid;
} setpgid_request_t;

typedef struct getsid_request {
    int64_t pid;
} getsid_request_t;

typedef struct ioctl_fioclex_request {
    int32_t fd;
} ioctl_fioclex_request_t;

typedef struct memfd_create_request {
    int32_t flags;
    char name[0];
} memfd_create_request_t;

typedef struct getpid_request {
} getpid_request_t;

typedef struct mkdirat_request {
    int32_t fd;
    uint32_t mode;
    char path[0];
} mkdirat_request_t;

typedef struct waitid_request {
    uint16_t idtype;
    uint64_t id;
    int32_t flags;
} waitid_request_t;

typedef struct readlinkat_request {
    int32_t fd;
    char path[0];
} readlinkat_request_t;

typedef struct sysconf_request {
    int32_t num;
} sysconf_request_t;

typedef struct reboot_request {
    int64_t cmd;
} reboot_request_t;

typedef struct fstatfs_request {
    int64_t fd;
    char path[0];
} fstatfs_request_t;

typedef struct parent_death_signal_request {
    int32_t signal;
} parent_death_signal_request_t;

typedef struct dup2_request {
    int32_t fd;
    int32_t newfd;
    int32_t flags;
    int32_t fcntl_mode;
} dup2_request_t;

typedef struct inotify_rm_request {
    int32_t ifd;
    int32_t wd;
} inotify_rm_request_t;

typedef struct timerfd_create_request {
    int32_t clock;
    int32_t flags;
} timerfd_create_request_t;

typedef struct timerfd_set_request {
    int32_t fd;
    int32_t flags;
    uint64_t value_sec;
    uint64_t value_nsec;
    uint64_t interval_sec;
    uint64_t interval_nsec;
} timerfd_set_request_t;

typedef struct timerfd_get_request {
    int32_t fd;
} timerfd_get_request_t;

typedef struct fchownat_request {
    int32_t fd;
    int32_t flags;
    int32_t uid;
    int32_t gid;
    char path[0];
} fchownat_request_t;

#define REQUEST_MAGIC 0x98765431

typedef struct request {
    uint64_t magic;
    uint64_t type;
    uint64_t opcode;
    uint64_t data_len;
    char data[];
} request_t;

typedef struct waitid_response {
    int error;
    int64_t pid;
    int64_t uid;
    uint64_t sig_status;
    uint64_t sig_code;
} waitid_response_t;

typedef struct readlinkat_response {
    int32_t fd;
    char path[0];
} readlinkat_response_t;

typedef struct sysconf_response {
    int error;
    int64_t value;
} sysconf_response_t;

typedef struct fstatfs_response {
    uint32_t fstype;
    uint64_t block_size;
    uint64_t fragment_size;
    uint64_t num_blocks;
    uint64_t blocks_free;
    uint64_t blocks_free_user;
    uint64_t num_inodes;
    uint64_t inodes_free;
    uint64_t inodes_free_user;
    uint64_t max_name_length;
    int32_t fsid0;
    int32_t fsid1;
    uint64_t flags;
} fstatfs_response_t;

typedef struct parent_death_signal_response {
    int error;
} parent_death_signal_response_t;

typedef struct dup2_response {
    int error;
    int32_t fd;
} dup2_response_t;

typedef struct timerfd_create_response {
    int error;
    uint32_t fd;
} timerfd_create_response_t;

typedef struct timerfd_set_response {
    int error;
} timerfd_set_response_t;

typedef struct timerfd_get_response {
    int error;
    uint64_t value_sec;
    uint64_t value_nsec;
    uint64_t interval_sec;
    uint64_t interval_nsec;
} timerfd_get_response_t;

typedef struct set_resource_limit_response {
    int error;
} set_resource_limit_response_t;

typedef struct fchownat_response {
    int error;
} fchownat_response_t;

#define RESPONSE_MAGIC 0x13486578

typedef struct response {
    uint64_t magic;
    uint64_t type;
    uint64_t res_code;
    uint64_t data_len;
    char data[];
} response_t;

#define MAX_SEND_ONCE 64

typedef struct sender {
    handle_id_t lane;
    k_action_t *actions;
    int idx;
} sender_t;

static inline sender_t *create_sender(handle_id_t lane) {
    sender_t *s = calloc(1, sizeof(sender_t));
    s->lane = lane;
    s->actions = calloc(MAX_SEND_ONCE, sizeof(k_action_t));
    return s;
}

static inline void sender_send_request(sender_t *sender, const request_t *buf) {
    size_t total_len = offsetof(request_t, data) + buf->data_len;
    sender->actions[sender->idx] = (k_action_t){
        .type = kActionSendFromBuffer,
        .buffer = (void *)buf,
        .length = total_len,
    };
    sender->idx++;
}

static inline void sender_send_response(sender_t *sender,
                                        const response_t *buf) {
    size_t total_len = offsetof(response_t, data) + buf->data_len;
    sender->actions[sender->idx] = (k_action_t){
        .type = kActionSendFromBuffer,
        .buffer = (void *)buf,
        .length = total_len,
    };
    sender->idx++;
}

static inline k_error_t sender_send(sender_t *sender) {
    k_error_t error = kSubmitDescriptor(sender->lane, sender->actions,
                                        sender->idx, KCALL_SUBMIT_NO_RECEIVING);
    sender->idx = 0;
    return error;
}

typedef struct receiver {
    handle_id_t lane;
    k_action_t *actions;
    int idx;
} receiver_t;

static inline receiver_t *create_receiver(handle_id_t lane) {
    receiver_t *r = calloc(1, sizeof(receiver_t));
    r->lane = lane;
    r->actions = calloc(MAX_SEND_ONCE, sizeof(k_action_t));
    return r;
}

static inline void receiver_recv_request(receiver_t *receiver, request_t *buf) {
    size_t total_len = offsetof(request_t, data);
    receiver->actions[receiver->idx] = (k_action_t){
        .type = kActionRecvToBuffer,
        .buffer = buf,
        .length = total_len,
    };
    receiver->idx++;
}

static inline void receiver_recv_response(receiver_t *receiver,
                                          response_t *buf) {
    size_t total_len = offsetof(request_t, data) + buf->data_len;
    receiver->actions[receiver->idx] = (k_action_t){
        .type = kActionRecvToBuffer,
        .buffer = buf,
        .length = total_len,
    };
    receiver->idx++;
}

static inline void receiver_recv_data(receiver_t *receiver, void *buf,
                                      size_t len) {
    receiver->actions[receiver->idx] = (k_action_t){
        .type = kActionRecvToBuffer,
        .buffer = buf,
        .length = len,
    };
    receiver->idx++;
}

static inline k_error_t receiver_recv(receiver_t *receiver) {
    k_error_t error =
        kSubmitDescriptor(receiver->lane, receiver->actions, receiver->idx, 0);
    receiver->idx = 0;
    return error;
}
