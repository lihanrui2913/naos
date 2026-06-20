#include "arch.h"
#include <drivers/logger.h>
#include <libs/klibc.h>
#include <mm/mm.h>
#include <task/task.h>
#include <irq/irq_manager.h>

#define X64_TLB_SHOOTDOWN_QUEUE_SIZE 32

typedef struct x64_tlb_shootdown_req {
    uint64_t seq;
    uint64_t mm_pgdir;
    uint64_t vaddr;
    bool all;
} x64_tlb_shootdown_req_t;

typedef struct x64_tlb_shootdown_state {
    spinlock_t lock;
    uint32_t head;
    uint32_t tail;
    bool force_all;
    uint64_t force_seq;
    uint64_t next_seq;
    uint64_t completed_seq;
    x64_tlb_shootdown_req_t queue[X64_TLB_SHOOTDOWN_QUEUE_SIZE];
} x64_tlb_shootdown_state_t;

static x64_tlb_shootdown_state_t x64_tlb_shootdown[MAX_CPU_NUM];

uint64_t arch_page_table_levels() { return ARCH_MAX_PT_LEVEL; }

uint64_t arch_user_va_limit(void) { return 0x00007fffffffffffULL; }

uint64_t *get_current_page_dir(bool user) {
    uint64_t page_table_base = 0;
    (void)user;
    asm volatile("movq %%cr3, %0" : "=r"(page_table_base));
    return (uint64_t *)phys_to_virt(page_table_base);
}

void set_current_page_dir(bool user, uint64_t pgdir) {
    (void)user;
    asm volatile("movq %0, %%cr3" ::"r"(pgdir) : "memory");
}

void arch_page_table_init(void) {
    memset(get_current_page_dir(false), 0, PAGE_SIZE / 2);
}

uint64_t arch_page_table_root_entries(int level) {
    uint64_t entries = (1UL << ARCH_PT_OFFSET_PER_LEVEL);
    if ((uint64_t)level == arch_page_table_levels())
        entries >>= 1;
    return entries;
}

uint64_t arch_make_page_table_entry(uint64_t paddr, uint64_t flags) {
    return ARCH_MAKE_PDE(paddr,
                         ARCH_PT_TABLE_FLAGS | (flags & ARCH_PT_FLAG_USER));
}

void arch_page_table_prepare_new(uint64_t *root) {
    memset(root, 0, PAGE_SIZE);
    arch_page_table_copy_kernel(root, get_kernel_page_dir());
}

void arch_page_table_copy_kernel(uint64_t *dst, uint64_t *src) {
    uint64_t user_entries = (1UL << ARCH_PT_OFFSET_PER_LEVEL) >> 1;
    memcpy(dst + user_entries, src + user_entries, PAGE_SIZE / 2);
}

bool arch_page_table_flags_writable(uint64_t flags) {
    return (flags & ARCH_PT_FLAG_WRITEABLE) != 0;
}

uint64_t arch_page_table_flags_make_cow(uint64_t flags) {
    return (flags | ARCH_PT_FLAG_COW) & ~ARCH_PT_FLAG_WRITEABLE;
}

uint64_t arch_page_table_flags_make_writable(uint64_t flags) {
    return (flags | ARCH_PT_FLAG_WRITEABLE) & ~ARCH_PT_FLAG_COW;
}

uint64_t get_arch_page_table_flags(uint64_t flags) {
    uint64_t result = ARCH_PT_FLAG_VALID;

    if ((flags & PT_FLAG_W) != 0) {
        result |= ARCH_PT_FLAG_WRITEABLE;
    }

    if ((flags & PT_FLAG_U) != 0) {
        result |= ARCH_PT_FLAG_USER;
    }

    if ((flags & PT_FLAG_X) == 0) {
        result |= ARCH_PT_FLAG_NX;
    }

    if ((flags & PT_FLAG_UNCACHEABLE) != 0 || (flags & PT_FLAG_DEVICE) != 0) {
        result |= (ARCH_PT_FLAG_PCD | ARCH_PT_FLAG_PWT);
    }

    if ((flags & PT_FLAG_COW) != 0) {
        result |= ARCH_PT_FLAG_COW;
    }

    return result;
}

void arch_flush_tlb(uint64_t vaddr) {
    asm volatile("invlpg (%0)" ::"r"(PADDING_DOWN(vaddr, PAGE_SIZE))
                 : "memory");
}

void arch_flush_tlb_all() {
    uint64_t cr3;
    asm volatile("movq %%cr3, %0" : "=r"(cr3)::"memory");
    asm volatile("movq %0, %%cr3" ::"r"(cr3) : "memory");
}

static uint64_t x64_tlb_current_pgdir(void) {
    uint64_t cr3;
    asm volatile("movq %%cr3, %0" : "=r"(cr3)::"memory");
    return ARCH_READ_PTE(cr3);
}

static bool x64_tlb_shootdown_should_wait(void) {
    task_t *self = current_task;
    return arch_interrupt_enabled() && (!self || self->preempt_count == 0);
}

static uint64_t x64_tlb_shootdown_enqueue_cpu(uint32_t cpu_id,
                                              uint64_t mm_pgdir, uint64_t vaddr,
                                              bool all) {
    x64_tlb_shootdown_state_t *state = &x64_tlb_shootdown[cpu_id];

    spin_lock(&state->lock);

    uint64_t seq = ++state->next_seq;
    uint32_t next_tail =
        (uint32_t)((state->tail + 1) % X64_TLB_SHOOTDOWN_QUEUE_SIZE);

    if (next_tail == state->head) {
        /* Queue overflow falls back to a full local flush on the target CPU. */
        state->force_all = true;
        if (seq > state->force_seq)
            state->force_seq = seq;
    } else {
        state->queue[state->tail] = (x64_tlb_shootdown_req_t){
            .seq = seq,
            .mm_pgdir = mm_pgdir,
            .vaddr = vaddr,
            .all = all,
        };
        state->tail = next_tail;
    }

    spin_unlock(&state->lock);
    return seq;
}

static void x64_tlb_shootdown_process_cpu(uint32_t cpu_id) {
    if (cpu_id >= MAX_CPU_NUM)
        return;

    x64_tlb_shootdown_state_t *state = &x64_tlb_shootdown[cpu_id];
    x64_tlb_shootdown_req_t queue[X64_TLB_SHOOTDOWN_QUEUE_SIZE];
    size_t count = 0;
    bool force_all = false;
    uint64_t complete_seq = 0;

    spin_lock(&state->lock);

    while (state->head != state->tail && count < X64_TLB_SHOOTDOWN_QUEUE_SIZE) {
        queue[count++] = state->queue[state->head];
        state->head =
            (uint32_t)((state->head + 1) % X64_TLB_SHOOTDOWN_QUEUE_SIZE);
    }

    force_all = state->force_all;
    if (state->force_seq > complete_seq)
        complete_seq = state->force_seq;
    state->force_all = false;
    state->force_seq = 0;

    if (count == 0 && !force_all) {
        spin_unlock(&state->lock);
        return;
    }

    spin_unlock(&state->lock);

    if (force_all) {
        arch_flush_tlb_all();
    } else {
        uint64_t current_pgdir = x64_tlb_current_pgdir();
        bool flush_all = false;

        for (size_t i = 0; i < count; i++) {
            if (queue[i].seq > complete_seq)
                complete_seq = queue[i].seq;

            /*
             * Requests for a non-current mm are already covered by the CR3
             * reload that happened when this CPU left that address space.
             */
            if (queue[i].mm_pgdir != current_pgdir)
                continue;

            if (queue[i].all) {
                flush_all = true;
                break;
            }
        }

        if (flush_all) {
            arch_flush_tlb_all();
        } else {
            for (size_t i = 0; i < count; i++) {
                if (queue[i].mm_pgdir != current_pgdir)
                    continue;
                arch_flush_tlb(queue[i].vaddr);
            }
        }
    }

    __atomic_store_n(&state->completed_seq, complete_seq, __ATOMIC_RELEASE);
}

void apic_tlb_shootdown_handle(void) {
    x64_tlb_shootdown_process_cpu(current_cpu_id);
}

static bool x64_tlb_shootdown_wait_cpu(uint32_t self_cpu, uint32_t cpu,
                                       uint64_t seq) {
    x64_tlb_shootdown_state_t *state = &x64_tlb_shootdown[cpu];

    while (__atomic_load_n(&state->completed_seq, __ATOMIC_ACQUIRE) < seq) {
        x64_tlb_shootdown_process_cpu(self_cpu);
        arch_pause();
    }

    return true;
}

static bool x64_tlb_shootdown_mm(task_mm_info_t *mm, uint64_t vaddr, bool all) {
    if (!mm) {
        if (all)
            arch_flush_tlb_all();
        else
            arch_flush_tlb(vaddr);
        return true;
    }

    uint32_t self_cpu = current_cpu_id;
    bool wait = x64_tlb_shootdown_should_wait();
    bool any_target = false;
    bool targets[MAX_CPU_NUM];
    uint64_t target_seq[MAX_CPU_NUM];
    uint64_t mm_pgdir = mm->page_table_addr;
    memset(targets, 0, sizeof(targets));
    memset(target_seq, 0, sizeof(target_seq));

    if (all)
        arch_flush_tlb_all();
    else
        arch_flush_tlb(vaddr);

    for (size_t word = 0; word < MM_ACTIVE_CPU_WORDS; word++) {
        uint64_t mask =
            __atomic_load_n(&mm->active_cpu_mask[word], __ATOMIC_ACQUIRE);
        while (mask) {
            uint32_t bit = __builtin_ctzll(mask);
            uint32_t cpu = (uint32_t)(word * 64 + bit);
            mask &= ~(1ULL << bit);

            if (cpu >= cpu_count || cpu >= MAX_CPU_NUM || cpu == self_cpu)
                continue;

            target_seq[cpu] =
                x64_tlb_shootdown_enqueue_cpu(cpu, mm_pgdir, vaddr, all);
            targets[cpu] = true;
            any_target = true;
            irq_send_ipi(cpu, APIC_TLB_SHOOTDOWN_VECTOR);
        }
    }

    if (!any_target)
        return true;

    if (!wait)
        return false;

    bool complete = true;
    for (uint32_t cpu = 0; cpu < cpu_count && cpu < MAX_CPU_NUM; cpu++) {
        if (!targets[cpu])
            continue;

        if (!x64_tlb_shootdown_wait_cpu(self_cpu, cpu, target_seq[cpu]))
            complete = false;
    }

    return complete;
}

bool task_mm_flush_tlb_page(task_mm_info_t *mm, uint64_t vaddr) {
    return x64_tlb_shootdown_mm(mm, vaddr, false);
}

bool task_mm_flush_tlb_all(task_mm_info_t *mm) {
    return x64_tlb_shootdown_mm(mm, 0, true);
}
