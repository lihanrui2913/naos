#pragma once

#include <libs/llist_queue.h>
#include <task/task.h>

struct sched_entity {
    task_t *task;
    list_node_t *node;
    bool on_rq;
};

typedef struct rrs_scheduler {
    list_queue_t *sched_queue;
    struct sched_entity *idle;
    struct sched_entity *curr;
} rrs_t;

extern rrs_t schedulers[MAX_CPU_NUM];

void add_sched_entity(task_t *task, void *scheduler);
void remove_sched_entity(task_t *thread, void *scheduler);
task_t *sched_pick_next_task(void *scheduler);

void sched_init();
