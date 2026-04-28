#pragma once

#include <libs/klibc.h>

extern int dlmalloc_errno;

#define LACKS_UNISTD_H
#define LACKS_FCNTL_H
#define LACKS_SYS_PARAM_H
#define LACKS_SYS_MMAN_H
#define LACKS_STRINGS_H
#define LACKS_STRING_H
#define LACKS_SYS_TYPES_H
#define LACKS_ERRNO_H
#define LACKS_STDLIB_H
#define LACKS_SCHED_H
#define LACKS_TIME_H

#define USE_LOCKS 2
#define NO_MALLOC_STATS 1
#define HAVE_MMAP 0
#define HAVE_MREMAP 0
#define HAVE_MORECORE 1
#define MORECORE sbrk
#define MORECORE_CONTIGUOUS 1
#define MORECORE_CANNOT_TRIM 1

#define MLOCK_T spinlock_t

extern MLOCK_T malloc_global_mutex;
int ACQUIRE_LOCK(MLOCK_T *lock);
int RELEASE_LOCK(MLOCK_T *lock);
int INITIAL_LOCK(MLOCK_T *lock);

void *sbrk(ptrdiff_t increment);

void abort();
