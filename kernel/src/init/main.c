#include <libs/klibc.h>
#include <boot/boot.h>
#include <drivers/kernel_logger.h>
#include <mm/mm.h>
#include <mm/heap.h>
#include <arch/arch.h>
#include <irq/irq_manager.h>
#include <drivers/tty.h>
#include <task/sched.h>
#include <task/task.h>

extern void acpi_init();

void kmain(void) {
    arch_disable_interrupt();

    boot_init();

    frame_init();

    page_table_init();

    heap_init_alloc();

    irq_manager_init();

    tty_init();

    acpi_init();

    arch_early_init();

    printk("Aether-OS starting...\n");

    sched_init();

    task_init();

    arch_init();

    while (1) {
        arch_enable_interrupt();
        arch_wait_for_interrupt();
    }
}
