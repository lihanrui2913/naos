#pragma once

#include <libs/klibc.h>

struct kstat {
    unsigned long int st_dev;
    unsigned long int st_ino;
    unsigned long int st_nlink;

    uint32_t st_mode;
    uint32_t st_uid;
    uint32_t st_gid;
    unsigned int __pad0;
    unsigned long int st_rdev;
    long int st_size;
    long int st_blksize;
    long int st_blocks;

    long st_atime_sec;
    long st_atime_nsec;
    long st_mtime_sec;
    long st_mtime_nsec;
    long st_ctime_sec;
    long st_ctime_nsec;
    long __unused[3];
};

struct dirent {
    unsigned long int d_ino;
    unsigned long int d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};
