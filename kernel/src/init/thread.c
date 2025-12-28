#include <arch/arch.h>
#include <task/task.h>

#include <drivers/bus/pci.h>
#include <drivers/fb.h>

extern void acpi_init_after_pci();

extern void sysfs_init();
extern void sysfs_init_umount();
extern void fsfdfs_init();
extern void cgroupfs_init();
extern void notifyfs_init();
extern void fs_syscall_init();
extern void socketfs_init();
extern void pipefs_init();

extern void mount_root();

bool system_initialized = false;

extern bool can_schedule;

void init_thread() {
    printk("NAOS init thread is running...\n");

    arch_init_after_thread();

    pci_controller_init();

#if !defined(__x86_64__)
    fdt_init();
#endif

    system_initialized = true;

    printk("System initialized, ready to go to userland.\n");

    //     const char *argvs[2];
    //     memset(argvs, 0, sizeof(argvs));
    //     argvs[0] = "/init";
    //     task_execve("/init", argvs, NULL);

    printk("run init failed\n");

    while (1) {
        arch_enable_interrupt();
        arch_wait_for_interrupt();
    }
}
