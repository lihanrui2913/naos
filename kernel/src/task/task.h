#pragma once

#include <arch/arch.h>
#include <libs/klibc.h>
#include <libs/llist.h>
#include <mm/bitmap.h>
#include <mm/mm.h>
#include <task/universe.h>

extern struct llist_header tasks_list;
extern struct llist_header default_blocking_list;
extern spinlock_t tasks_list_lock;
extern uint64_t next_pid;
extern spinlock_t pid_alloc_lock;
extern uint64_t next_cpuid;
extern spinlock_t cpuid_alloc_lock;

#define K_SIGNAL_SEGMENTATION_FAULT (128 + 1)

typedef enum task_state {
    TASK_READY = 1,
    TASK_RUNNING,
    TASK_BLOCKING,
    TASK_DIED,
} task_state_t;

typedef struct task {
    uint64_t kernel_stack;
    struct llist_header node;
    uint64_t cpu_id;
    bool user;
    uint64_t pid;
    struct task *parent;
    int *futex_pointer;
    uint64_t force_wakeup_time;
    uint64_t cap;
    char *name;
    task_state_t state;
    char *block_reason;
    task_arch_info_t *arch;
    task_mm_info_t *mm;
    universe_t *universe;
    void *sched_info;
} task_t;

extern task_t *arch_get_current();
#define current_task arch_get_current()

extern bool can_schedule;
#define SCHED_NONE 0
#define SCHED_YIELD (1UL << 0)
void schedule(uint64_t flags);

void task_free(task_t *task);
void task_exit(uint64_t code);

void task_block(task_t *task, struct llist_header *blocking_list,
                uint64_t timeout, const char *reason);
void task_unblock(task_t *task);

task_t *task_create(const char *name, uint64_t cap, int priority,
                    uint64_t entry, uint64_t arg, bool is_idle);
task_t *task_create_user(universe_t *universe, void *ip, void *sp,
                         uint64_t flags);

void task_init();
