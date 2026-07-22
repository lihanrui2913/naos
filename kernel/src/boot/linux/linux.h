#pragma once

#include <boot/boot.h>
#include <boot/linux/bootparam.h>

#define LINUX_HHDM_OFFSET 0xffff800000000000ULL
#define LINUX_DIRECT_MAP_SIZE (512ULL * 1024 * 1024 * 1024)
#define LINUX_KERNEL_VIRT_OFFSET 0xffffffff80000000ULL
#define LINUX_KERNEL_LOAD_ADDRESS 0x100000ULL
#define LINUX_AP_TRAMPOLINE_ADDRESS 0x8000ULL

typedef struct linux_boot_context {
    struct linux_boot_params *params;
    uintptr_t params_phys;
    uintptr_t acpi_rsdp;
    uintptr_t smbios32;
    uintptr_t smbios64;
    uint64_t boot_time;
} linux_boot_context_t;

void linux_init(uintptr_t boot_params_phys);
linux_boot_context_t *linux_context(void);
void *linux_phys_to_virt(uint64_t phys);

uint64_t linux_read_boot_time(void);
void linux_find_smbios(uintptr_t *entry32, uintptr_t *entry64);
