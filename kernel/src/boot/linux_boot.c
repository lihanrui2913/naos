#include <boot/boot.h>
#include <boot/linux/linux.h>
#include <acpi/uacpi/acpi.h>
#include <arch/x86_64/core/normal.h>
#include <drivers/logger.h>
#include <mm/mm.h>

#define LINUX_MAX_MADT_CPUS MAX_CPU_NUM
#define LINUX_PAGE_PRESENT 1ULL
#define LINUX_PAGE_WRITABLE 2ULL
#define LINUX_EFI32_SIGNATURE 0x32334c45U
#define LINUX_EFI64_SIGNATURE 0x34364c45U

extern uint64_t linux_boot_params_phys;
extern char __linux_payload_memory_end[];
extern uint64_t linux_identity_pdpt[];

extern char linux_ap_trampoline_start[];
extern char linux_ap_trampoline_end[];
extern char linux_ap_cr3[];
extern char linux_ap_stack[];
extern char linux_ap_entry[];

extern uint64_t cpu_count;
extern uint32_t cpuid_to_lapicid[MAX_CPU_NUM];
extern spinlock_t ap_startup_lock;

static boot_memory_map_t memory_map;
static boot_framebuffer_t framebuffer;
static boot_module_t initrd_module;
static uint32_t madt_cpu_ids[LINUX_MAX_MADT_CPUS];
static size_t madt_cpu_count;

static void memory_map_append(uint64_t addr, uint64_t size,
                              typeof(memory_map.entries[0].type) type) {
    if (!size || memory_map.entry_count >=
                     sizeof(memory_map.entries) / sizeof(memory_map.entries[0]))
        return;

    memory_map.entries[memory_map.entry_count++] = (boot_memory_map_entry_t){
        .addr = addr,
        .len = size,
        .type = type,
    };
}

struct reserved_span {
    uint64_t start;
    uint64_t end;
};

static void memory_map_append_usable(uint64_t start, uint64_t end,
                                     struct reserved_span *spans,
                                     size_t span_count) {
    uint64_t cursor = start;

    for (size_t i = 0; i < span_count && cursor < end; i++) {
        uint64_t reserved_start = spans[i].start;
        uint64_t reserved_end = spans[i].end;
        if (reserved_end <= cursor || reserved_start >= end)
            continue;
        if (reserved_start > cursor)
            memory_map_append(cursor, reserved_start - cursor, USABLE);
        if (reserved_end > cursor)
            cursor = reserved_end;
    }
    if (cursor < end)
        memory_map_append(cursor, end - cursor, USABLE);
}

static void build_memory_map(void) {
    struct linux_boot_params *params = linux_context()->params;
    uint64_t initrd_start = params->hdr.ramdisk_image;
    uint64_t initrd_end = initrd_start + params->hdr.ramdisk_size;
    struct reserved_span spans[2] = {
        {.start = LINUX_KERNEL_LOAD_ADDRESS,
         .end = (uintptr_t)__linux_payload_memory_end},
        {.start = initrd_start, .end = initrd_end},
    };

    if (spans[1].start < spans[0].start) {
        struct reserved_span tmp = spans[0];
        spans[0] = spans[1];
        spans[1] = tmp;
    }

    memset(&memory_map, 0, sizeof(memory_map));
    for (size_t i = 0; i < params->e820_entries && i < LINUX_E820_MAX_ENTRIES;
         i++) {
        struct linux_e820_entry *entry = &params->e820_table[i];
        uint64_t start = entry->addr;
        uint64_t end = entry->addr + entry->size;
        if (end < start || start >= LINUX_DIRECT_MAP_SIZE)
            continue;
        if (end > LINUX_DIRECT_MAP_SIZE)
            end = LINUX_DIRECT_MAP_SIZE;

        if (entry->type == LINUX_E820_TYPE_RAM)
            memory_map_append_usable(start, end, spans, 2);
        else
            memory_map_append(start, end - start, RESERVED);
    }

    memory_map_append(spans[0].start, spans[0].end - spans[0].start, RESERVED);
    if (spans[1].end > spans[1].start)
        memory_map_append(spans[1].start, spans[1].end - spans[1].start,
                          RESERVED);
}

void boot_init(void) {
    linux_init((uintptr_t)linux_boot_params_phys);
    build_memory_map();
}

uint64_t boot_get_hhdm_offset(void) { return LINUX_HHDM_OFFSET; }

boot_memory_map_t *boot_get_memory_map(void) { return &memory_map; }

uintptr_t boot_get_acpi_rsdp(void) {
    linux_boot_context_t *ctx = linux_context();
    return ctx->acpi_rsdp;
}

void boot_get_smbios_entries(void **entry32, void **entry64) {
    linux_boot_context_t *ctx = linux_context();
    if (entry32)
        *entry32 = linux_phys_to_virt(ctx->smbios32);
    if (entry64)
        *entry64 = linux_phys_to_virt(ctx->smbios64);
}

uint64_t boot_get_boottime(void) { return linux_context()->boot_time; }

static void register_cpu(uint32_t apic_id, uint32_t flags) {
    if (!(flags & (ACPI_PIC_ENABLED | ACPI_PIC_ONLINE_CAPABLE)) ||
        apic_id > UINT8_MAX)
        return;

    for (size_t i = 0; i < madt_cpu_count; i++) {
        if (madt_cpu_ids[i] == apic_id)
            return;
    }
    if (madt_cpu_count < LINUX_MAX_MADT_CPUS)
        madt_cpu_ids[madt_cpu_count++] = apic_id;
}

void apic_handle_lapic(struct acpi_madt_lapic *lapic) {
    register_cpu(lapic->id, lapic->flags);
}

void apic_handle_lx2apic(struct acpi_madt_x2apic *lapic) {
    register_cpu(lapic->id, lapic->flags);
}

static bool wait_icr_idle(void) {
    for (size_t i = 0; i < 1000000; i++) {
        if (!(lapic_read(LAPIC_ICR_LOW) & ICR_DELIVERY_PENDING))
            return true;
        __asm__ volatile("pause");
    }
    return false;
}

static void smp_delay(uint64_t ns) {
    uint64_t start = hpet_nano_time();
    if (start) {
        while (hpet_nano_time() - start < ns)
            __asm__ volatile("pause");
        return;
    }

    for (volatile size_t i = 0; i < ns / 10 + 1000; i++)
        __asm__ volatile("pause");
}

static bool send_startup_ipi(uint32_t apic_id) {
    uint32_t destination = apic_id << 24;
    uint32_t init = ICR_DELIVERY_INIT | ICR_DEST_PHYSICAL | ICR_LEVEL_ASSERT |
                    ICR_TRIGGER_LEVEL | ICR_DEST_NOSHORTHAND;
    uint32_t startup = ICR_DELIVERY_STARTUP | ICR_DEST_PHYSICAL |
                       ICR_DEST_NOSHORTHAND |
                       (LINUX_AP_TRAMPOLINE_ADDRESS >> 12);

    if (!wait_icr_idle())
        return false;
    lapic_write(LAPIC_ICR_HIGH, destination);
    lapic_write(LAPIC_ICR_LOW, init);
    if (!wait_icr_idle())
        return false;
    smp_delay(10 * 1000 * 1000ULL);

    for (size_t i = 0; i < 2; i++) {
        lapic_write(LAPIC_ICR_HIGH, destination);
        lapic_write(LAPIC_ICR_LOW, startup);
        if (!wait_icr_idle())
            return false;
        smp_delay(200 * 1000ULL);
    }
    return true;
}

static void patch_trampoline(void *trampoline, const char *field,
                             const void *value, size_t size) {
    size_t offset = (size_t)(field - linux_ap_trampoline_start);
    memcpy((uint8_t *)trampoline + offset, value, size);
}

void boot_smp_init(uintptr_t entry) {
    uint32_t bsp_id = (uint32_t)lapic_id();
    size_t trampoline_size =
        (size_t)(linux_ap_trampoline_end - linux_ap_trampoline_start);
    void *trampoline = phys_to_virt(LINUX_AP_TRAMPOLINE_ADDRESS);
    uint64_t *pml4 = get_kernel_page_dir();
    uint64_t identity_pdpt_phys = virt_to_phys(linux_identity_pdpt);
    uint64_t cr3 = virt_to_phys(pml4);

    if (!trampoline || trampoline_size > PAGE_SIZE || cr3 > UINT32_MAX ||
        identity_pdpt_phys > UINT32_MAX) {
        cpu_count = 1;
        cpuid_to_lapicid[0] = bsp_id;
        return;
    }

    cpu_count = 1;
    cpuid_to_lapicid[0] = bsp_id;
    for (size_t i = 0; i < madt_cpu_count && cpu_count < MAX_CPU_NUM; i++) {
        if (madt_cpu_ids[i] != bsp_id)
            cpuid_to_lapicid[cpu_count++] = madt_cpu_ids[i];
    }

    memcpy(trampoline, linux_ap_trampoline_start, trampoline_size);
    pml4[0] = identity_pdpt_phys | LINUX_PAGE_PRESENT | LINUX_PAGE_WRITABLE;
    arch_flush_tlb_all();

    uint32_t trampoline_cr3 = (uint32_t)cr3;
    patch_trampoline(trampoline, linux_ap_cr3, &trampoline_cr3,
                     sizeof(trampoline_cr3));

    for (size_t i = 1; i < cpu_count; i++) {
        uintptr_t stack_phys = alloc_frames(STACK_SIZE / PAGE_SIZE);
        if (stack_phys == UINT64_MAX) {
            cpu_count = i;
            break;
        }
        uint64_t stack =
            (uint64_t)(uintptr_t)phys_to_virt(stack_phys + STACK_SIZE);
        uint64_t ap_entry = entry;
        patch_trampoline(trampoline, linux_ap_stack, &stack, sizeof(stack));
        patch_trampoline(trampoline, linux_ap_entry, &ap_entry,
                         sizeof(ap_entry));

        raw_spin_lock(&ap_startup_lock);
        if (!send_startup_ipi(cpuid_to_lapicid[i])) {
            raw_spin_unlock(&ap_startup_lock);
            cpu_count = i;
            break;
        }
    }

    if (cpu_count > 1) {
        raw_spin_lock(&ap_startup_lock);
        raw_spin_unlock(&ap_startup_lock);
    }
    pml4[0] = 0;
    arch_flush_tlb_all();
}

bool boot_cpu_support_x2apic(void) {
    /* The native INIT/SIPI path currently targets 8-bit xAPIC IDs. */
    return false;
}

boot_framebuffer_t *boot_get_framebuffer(void) {
    struct linux_boot_params *params = linux_context()->params;
    struct linux_screen_info *screen;
    uint64_t address;

    if (!params)
        return NULL;
    screen = &params->screen_info;
    if ((screen->orig_video_is_vga != LINUX_VIDEO_TYPE_VLFB &&
         screen->orig_video_is_vga != LINUX_VIDEO_TYPE_EFI) ||
        !screen->lfb_base || !screen->lfb_width || !screen->lfb_height ||
        !screen->lfb_depth)
        return NULL;

    address = screen->lfb_base;
    if (screen->capabilities & LINUX_VIDEO_CAPABILITY_64BIT_BASE)
        address |= (uint64_t)screen->ext_lfb_base << 32;
    framebuffer = (boot_framebuffer_t){
        .address = (uintptr_t)linux_phys_to_virt(address),
        .width = screen->lfb_width,
        .height = screen->lfb_height,
        .bpp = screen->lfb_depth,
        .pitch = screen->lfb_linelength,
        .red_mask_size = screen->red_size,
        .red_mask_shift = screen->red_pos,
        .green_mask_size = screen->green_size,
        .green_mask_shift = screen->green_pos,
        .blue_mask_size = screen->blue_size,
        .blue_mask_shift = screen->blue_pos,
    };
    return framebuffer.address ? &framebuffer : NULL;
}

char *boot_get_cmdline(void) {
    struct linux_boot_params *params = linux_context()->params;
    uint64_t address;
    if (!params)
        return "";
    address = (uint64_t)params->hdr.cmd_line_ptr |
              ((uint64_t)params->ext_cmd_line_ptr << 32);
    char *cmdline = linux_phys_to_virt(address);
    return cmdline ? cmdline : "";
}

void *boot_get_executable_file(size_t *size) {
    if (size)
        *size = 0;
    return NULL;
}

void boot_get_modules(boot_module_t **modules, size_t *count) {
    struct linux_boot_params *params = linux_context()->params;
    uint64_t address;

    *count = 0;
    if (!params || !params->hdr.ramdisk_image || !params->hdr.ramdisk_size)
        return;

    address = (uint64_t)params->hdr.ramdisk_image |
              ((uint64_t)params->ext_ramdisk_image << 32);
    memset(&initrd_module, 0, sizeof(initrd_module));
    strcpy(initrd_module.path, "initramfs.img");
    initrd_module.data = linux_phys_to_virt(address);
    initrd_module.size = (uint64_t)params->hdr.ramdisk_size |
                         ((uint64_t)params->ext_ramdisk_size << 32);
    if (!initrd_module.data)
        return;
    modules[0] = &initrd_module;
    *count = 1;
}

uint64_t boot_get_firmware_type(void) {
    struct linux_boot_params *params = linux_context()->params;
    if (!params)
        return 0;
    if (params->efi_info.loader_signature == LINUX_EFI64_SIGNATURE)
        return 2;
    if (params->efi_info.loader_signature == LINUX_EFI32_SIGNATURE)
        return 1;
    return 0;
}

uint64_t boot_get_system_table(void) {
    struct linux_boot_params *params = linux_context()->params;
    uint64_t address;
    if (!params)
        return 0;
    address = (uint64_t)params->efi_info.system_table |
              ((uint64_t)params->efi_info.system_table_hi << 32);
    return (uint64_t)(uintptr_t)linux_phys_to_virt(address);
}
