#pragma once

#include <libs/klibc.h>
#include <libs/llist.h>

struct vfs_fd;

typedef ssize_t (*vfs_read_t)(struct vfs_fd *fd, uint8_t *out, size_t limit);
typedef ssize_t (*vfs_write_t)(struct vfs_fd *fd, const uint8_t *in,
                               size_t limit);
typedef ssize_t (*vfs_seek_t)(struct vfs_fd *fd, long int offset, int whence);
typedef ssize_t (*vfs_ioctl_t)(struct vfs_fd *fd, uint64_t request, void *arg);
typedef ssize_t (*vfs_stat_t)(struct vfs_fd *fd, struct kstat *stat);
typedef ssize_t (*vfs_readdir_t)(struct vfs_fd *fd, struct dirent *dents);
typedef ssize_t (*vfs_open_t)(char *fn, int flags, int mode, struct vfs_fd *fd,
                              char **symlink_resolve);
typedef bool (*vfs_close_t)(struct vfs_fd *fd);
typedef size_t (*vfs_get_filesize_t)(struct vfs_fd *fd);
typedef void (*vfs_fcntl_t)(struct vfs_fd *fd, int cmd, uint64_t arg);
typedef size_t (*vfs_poll_state_t)(struct vfs_fd *fd, size_t events);
typedef bool (*vfs_poll_wait_t)(struct vfs_fd *fd, size_t events,
                                size_t *revents, int timeout);
typedef size_t (*vfs_bind_t)(struct vfs_fd *fd, void *addr, size_t len);
typedef size_t (*vfs_listen_t)(struct vfs_fd *fd, int backlog);
typedef size_t (*vfs_accept_t)(struct vfs_fd *fd, void *addr, int *len);
typedef size_t (*vfs_connect_t)(struct vfs_fd *fd, void *addr, int len);
typedef size_t (*vfs_recvfrom_t)(struct vfs_fd *fd, uint8_t *out, size_t limit,
                                 int flags, void *addr, uint32_t *len);
typedef size_t (*vfs_sendto_t)(struct vfs_fd *fd, uint8_t *in, size_t limit,
                               int flags, void *addr, uint32_t len);
typedef size_t (*vfs_getsockopts_t)(struct vfs_fd *fd, int level, int optname,
                                    void *optval, uint32_t *socklen);
typedef size_t (*vfs_setsockopts_t)(struct vfs_fd *fd, int level, int optname,
                                    const void *optval, uint32_t socklen);
typedef size_t (*vfs_getsockname_t)(struct vfs_fd *fd, void *addr,
                                    uint32_t *addrlen);
typedef size_t (*vfs_getpeername_t)(struct vfs_fd *fd, void *addr,
                                    uint32_t *len);
typedef size_t (*vfs_recv_msg_t)(struct vfs_fd *fd, struct msghdr_linux *msg,
                                 int flags);
typedef size_t (*vfs_send_msg_t)(struct vfs_fd *fd, struct msghdr_linux *msg,
                                 int flags);

typedef struct vfs_op {
    // general operations
    vfs_read_t read;
    vfs_write_t write;
    vfs_seek_t seek;
    vfs_ioctl_t ioctl;
    vfs_stat_t stat;
    vfs_readdir_t readdir;
    vfs_get_filesize_t get_filesize;
    vfs_poll_state_t poll_state;
    vfs_poll_wait_t poll_wait;
    vfs_fcntl_t fcntl;

    // networking
    vfs_bind_t bind;
    vfs_listen_t listen;
    vfs_accept_t accept;
    vfs_connect_t connect;
    vfs_recvfrom_t recvfrom;
    vfs_sendto_t sendto;
    vfs_getsockopts_t getsockopts;
    vfs_setsockopts_t setsockopts;
    vfs_getpeername_t getpeername;
    vfs_getsockname_t getsockname;

    // file state
    vfs_open_t open;
    vfs_close_t close;
} vfs_op_t;

typedef struct vfs_mp {
    struct llist_header node;

    char *prefix;

    vfs_op_t *ops;

    void *private;
} vfs_mp_t;

typedef struct vfs_fd {
    int id;

    spinlock_t op_lock;

    int flags;
    int mode;

    bool close_on_exec;

    void *private;

    vfs_op_t *ops;
    vfs_mp_t *mp;
} vfs_fd_t;

extern struct llist_header mount_points;

#define SEEK_SET 0
#define SEEK_CURR 1
#define SEEK_END 2

void vfs_init();
