#include <arch/arch.h>
#include <task/task.h>

#include <drivers/bus/pci.h>
#include <drivers/fb.h>

#include <init/initramfs.h>

#include <libs/elf.h>

#define USER_STACK_END 0x00007ffffffff000
#define USER_STACK_START 0x00007fffff000000

void load_elf(const char *path) {
    initramfs_handle_t *handle = initramfs_lookup("posix-subsystem");
    if (!handle)
        return;
    Elf64_Ehdr ehdr;
    initramfs_read(handle, &ehdr, 0, sizeof(Elf64_Ehdr));

    uint64_t e_entry = ehdr.e_entry;
    if (!e_entry)
        return;

    _cleanup_free_ Elf64_Phdr *phdrs = calloc(ehdr.e_phnum, sizeof(Elf64_Phdr));
    initramfs_read(handle, phdrs, ehdr.e_phoff,
                   ehdr.e_phnum * sizeof(Elf64_Phdr));
    for (Elf64_Half i = 0; i < ehdr.e_phnum; i++) {
        Elf64_Phdr *phdr = &phdrs[i];
        if (phdr->p_type != PT_LOAD)
            continue;
        map_page_range(get_current_page_dir(false), phdr->p_vaddr, 0,
                       PADDING_UP(phdr->p_memsz, DEFAULT_PAGE_SIZE),
                       PT_FLAG_R | PT_FLAG_W | PT_FLAG_U | PT_FLAG_X);
        initramfs_read(handle, (void *)phdr->p_vaddr, phdr->p_offset,
                       phdr->p_filesz);
        if (phdr->p_memsz > phdr->p_filesz) {
            memset((void *)(phdr->p_vaddr + phdr->p_filesz), 0,
                   phdr->p_memsz - phdr->p_filesz);
        }
    }
    map_page_range(get_current_page_dir(false), USER_STACK_START, 0,
                   USER_STACK_END - USER_STACK_START,
                   PT_FLAG_R | PT_FLAG_W | PT_FLAG_U);

    universe_t *u = create_universe();
    task_create_user(u, (void *)e_entry, (void *)USER_STACK_END, 0);
    drop_universe(u);

    for (Elf64_Half i = 0; i < ehdr.e_phnum; i++) {
        Elf64_Phdr *phdr = &phdrs[i];
        if (phdr->p_type != PT_LOAD)
            continue;
        unmap_page_range(get_current_page_dir(false), phdr->p_vaddr,
                         PADDING_UP(phdr->p_memsz, DEFAULT_PAGE_SIZE));
    }
    unmap_page_range(get_current_page_dir(false), USER_STACK_START,
                     USER_STACK_END - USER_STACK_START);
}

extern void acpi_init_after_pci();

bool system_initialized = false;

extern bool can_schedule;

void init_thread() {
    printk("NAOS init thread is running...\n");

    arch_init_after_thread();

    pci_controller_init();

#if !defined(__x86_64__)
    fdt_init();
#endif

    initramfs_init();

    system_initialized = true;

    printk("System initialized, ready to go to userland.\n");

    load_elf("/posix-subsystem");

    while (1) {
        arch_enable_interrupt();
        arch_wait_for_interrupt();
    }
}
