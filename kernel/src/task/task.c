#include <task/task.h>
#include <task/sched.h>

task_t *idle_tasks[MAX_CPU_NUM];

bool can_schedule = false;
bool task_initialized = false;

struct llist_header tasks_list;
spinlock_t tasks_list_lock = SPIN_INIT;
uint64_t next_pid = 1;
spinlock_t pid_alloc_lock = SPIN_INIT;
uint64_t next_cpuid = 0;
spinlock_t cpuid_alloc_lock = SPIN_INIT;

uint64_t allocate_cpu_id() {
    spin_lock(&cpuid_alloc_lock);
    uint64_t res = next_cpuid;
    next_cpuid = (next_cpuid + 1) % cpu_count;
    spin_unlock(&cpuid_alloc_lock);
    return res;
}

void task_free(task_t *task) {
    llist_delete(&task->node);
    if (task->name)
        free(task->name);
    if (task->block_reason)
        free(task->block_reason);
    drop_universe(task->universe);

    if (task->sched_info)
        free(task->sched_info);

    // task->arch
    // task->mm
}

void task_exit(uint64_t code) {
    remove_sched_entity(current_task, &schedulers[current_task->cpu_id]);
    current_task->state = TASK_DIED;
}

void schedule(uint64_t flags) {
    task_t *prev = current_task;
    spin_lock(&tasks_list_lock);
    task_t *next = sched_pick_next_task(&schedulers[current_cpu_id]);
    spin_unlock(&tasks_list_lock);
    if (next->state == TASK_DIED) {
        next = idle_tasks[current_cpu_id];
    }
    if (prev == next)
        goto ret;

    if (prev->state != TASK_DIED && prev->state != TASK_BLOCKING) {
        prev->state = TASK_READY;
    }
    next->state = TASK_RUNNING;

    arch_set_current(next);

    switch_to(prev, next);
ret:
}

task_mm_info_t *task_mm_create(task_t *task) {
    task_mm_info_t *mm = malloc(sizeof(task_mm_info_t));
    mm->page_table_addr = virt_to_phys(get_current_page_dir(false));
    mm->ref_count = 1;
    memset(&mm->task_vma_mgr, 0, sizeof(vma_manager_t));
    return mm;
}

task_t *task_create(const char *name, uint64_t cap, int priority,
                    uint64_t entry, uint64_t arg, bool is_idle) {
    task_t *task = malloc(sizeof(task_t));
    memset(task, 0, sizeof(task_t));
    task->kernel_stack = (uint64_t)alloc_frames_bytes(STACK_SIZE);
    llist_init_head(&task->node);
    task->cpu_id = allocate_cpu_id();
    task->user = false;
    spin_lock(&pid_alloc_lock);
    task->pid = !is_idle ? next_pid++ : 0;
    spin_unlock(&pid_alloc_lock);
    task->parent = current_task;
    task->cap = cap;
    task->name = name ? strdup(name) : NULL;
    task->state = TASK_READY;
    task->block_reason = NULL;
    task->mm = task_mm_create(task);
    task_arch_init(task, task->kernel_stack, entry, arg);

    task->universe = create_universe();

    spin_lock(&tasks_list_lock);
    llist_append(&tasks_list, &task->node);
    spin_unlock(&tasks_list_lock);

    task->sched_info = malloc(sizeof(struct sched_entity));
    memset(task->sched_info, 0, sizeof(struct sched_entity));
    add_sched_entity(task, &schedulers[task->cpu_id]);

    return task;
}

task_t *task_create_user(universe_t *universe, void *ip, void *sp,
                         uint64_t flags) {
    task_t *task = malloc(sizeof(task_t));
    memset(task, 0, sizeof(task_t));
    task->kernel_stack = (uint64_t)alloc_frames_bytes(STACK_SIZE);
    llist_init_head(&task->node);
    task->cpu_id = allocate_cpu_id();
    task->user = false;
    spin_lock(&pid_alloc_lock);
    task->pid = next_pid++;
    spin_unlock(&pid_alloc_lock);
    task->parent = current_task;
    task->cap = 0;
    task->name = strdup("USER");
    task->state = TASK_READY;
    task->block_reason = NULL;
    task->mm = task_mm_create(task);
    task_arch_init_user(task, task->kernel_stack, (uint64_t)ip, (uint64_t)sp);

    universe->refcount++;
    task->universe = universe;

    spin_lock(&tasks_list_lock);
    llist_append(&tasks_list, &task->node);
    spin_unlock(&tasks_list_lock);

    task->sched_info = malloc(sizeof(struct sched_entity));
    memset(task->sched_info, 0, sizeof(struct sched_entity));
    add_sched_entity(task, &schedulers[task->cpu_id]);

    return task;
}

void idle_entry() {
    while (1) {
        schedule(SCHED_YIELD);
    }
}

extern void init_thread();

void task_init() {
    llist_init_head(&tasks_list);
    for (uint64_t cpu = 0; cpu < cpu_count; cpu++) {
        idle_tasks[cpu] =
            task_create("idle", 0, 0, (uint64_t)idle_entry, 0, true);
    }
    arch_set_current(idle_tasks[0]);
    task_create("init", 0, 0, (uint64_t)init_thread, 0, false);
    task_initialized = true;
    can_schedule = true;
}
