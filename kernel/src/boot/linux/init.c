#include <boot/linux/linux.h>
#include <arch/x86_64/io.h>

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71
#define CMOS_NMI_DISABLE 0x80
#define CMOS_SECONDS 0x00
#define CMOS_MINUTES 0x02
#define CMOS_HOURS 0x04
#define CMOS_DAY 0x07
#define CMOS_MONTH 0x08
#define CMOS_YEAR 0x09
#define CMOS_STATUS_A 0x0a
#define CMOS_STATUS_B 0x0b
#define CMOS_UPDATE_IN_PROGRESS 0x80
#define CMOS_24_HOUR 0x02
#define CMOS_BINARY 0x04

static linux_boot_context_t context;

struct efi_guid {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
} __attribute__((packed));

struct efi_table_header {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
} __attribute__((packed));

struct efi_system_table64 {
    struct efi_table_header header;
    uint64_t firmware_vendor;
    uint32_t firmware_revision;
    uint32_t pad;
    uint64_t console_in_handle;
    uint64_t console_in;
    uint64_t console_out_handle;
    uint64_t console_out;
    uint64_t stderr_handle;
    uint64_t stderr;
    uint64_t runtime_services;
    uint64_t boot_services;
    uint64_t table_entry_count;
    uint64_t configuration_table;
} __attribute__((packed));

struct efi_configuration_table64 {
    struct efi_guid guid;
    uint64_t table;
} __attribute__((packed));

static const struct efi_guid acpi_20_table_guid = {
    0x8868e871,
    0xe4f1,
    0x11d3,
    {0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81},
};
static const struct efi_guid acpi_table_guid = {
    0xeb9d2d30,
    0x2d88,
    0x11d3,
    {0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d},
};
static const struct efi_guid smbios_table_guid = {
    0xeb9d2d31,
    0x2d88,
    0x11d3,
    {0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d},
};
static const struct efi_guid smbios3_table_guid = {
    0xf2fd1544,
    0x9794,
    0x4a2c,
    {0x99, 0x2e, 0xe5, 0xbb, 0xcf, 0x20, 0xe3, 0x94},
};

void *linux_phys_to_virt(uint64_t phys) {
    if (phys == 0 || phys >= LINUX_DIRECT_MAP_SIZE)
        return NULL;
    return (void *)(uintptr_t)(LINUX_HHDM_OFFSET + phys);
}

static uint8_t cmos_read(uint8_t reg) {
    io_out8(CMOS_ADDRESS, CMOS_NMI_DISABLE | reg);
    return io_in8(CMOS_DATA);
}

static uint8_t bcd_to_binary(uint8_t value) {
    return (uint8_t)((value & 0x0f) + (value >> 4) * 10);
}

static bool leap_year(uint32_t year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

static uint64_t calendar_to_unix(uint32_t year, uint32_t month, uint32_t day,
                                 uint32_t hour, uint32_t minute,
                                 uint32_t second) {
    static const uint16_t days_before_month[] = {
        0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334,
    };
    uint64_t days = 0;

    if (year < 1970 || month == 0 || month > 12 || day == 0 || day > 31 ||
        hour > 23 || minute > 59 || second > 60)
        return 0;

    for (uint32_t y = 1970; y < year; y++)
        days += leap_year(y) ? 366 : 365;
    days += days_before_month[month] + day - 1;
    if (month > 2 && leap_year(year))
        days++;

    return days * 86400ULL + hour * 3600ULL + minute * 60ULL + second;
}

uint64_t linux_read_boot_time(void) {
    uint8_t second, minute, hour, day, month, year, status_b;

    while (cmos_read(CMOS_STATUS_A) & CMOS_UPDATE_IN_PROGRESS)
        __asm__ volatile("pause");

    second = cmos_read(CMOS_SECONDS);
    minute = cmos_read(CMOS_MINUTES);
    hour = cmos_read(CMOS_HOURS);
    day = cmos_read(CMOS_DAY);
    month = cmos_read(CMOS_MONTH);
    year = cmos_read(CMOS_YEAR);
    status_b = cmos_read(CMOS_STATUS_B);

    if (!(status_b & CMOS_BINARY)) {
        second = bcd_to_binary(second);
        minute = bcd_to_binary(minute);
        hour = (uint8_t)(bcd_to_binary(hour & 0x7f) | (hour & 0x80));
        day = bcd_to_binary(day);
        month = bcd_to_binary(month);
        year = bcd_to_binary(year);
    }

    if (!(status_b & CMOS_24_HOUR)) {
        bool pm = (hour & 0x80) != 0;
        hour &= 0x7f;
        if (pm && hour < 12)
            hour += 12;
        else if (!pm && hour == 12)
            hour = 0;
    }

    return calendar_to_unix(2000U + year, month, day, hour, minute, second);
}

static uint8_t checksum(const uint8_t *bytes, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++)
        sum = (uint8_t)(sum + bytes[i]);
    return sum;
}

static void scan_smbios_range(uintptr_t start, uintptr_t end,
                              uintptr_t *entry32, uintptr_t *entry64) {
    for (uintptr_t phys = start; phys + 0x20 <= end; phys += 16) {
        const uint8_t *entry = linux_phys_to_virt(phys);
        if (!entry)
            break;

        if (!*entry64 && memcmp(entry, "_SM3_", 5) == 0 && entry[6] >= 0x18 &&
            phys + entry[6] <= end && checksum(entry, entry[6]) == 0)
            *entry64 = phys;

        if (!*entry32 && memcmp(entry, "_SM_", 4) == 0 && entry[5] >= 0x1f &&
            phys + entry[5] <= end && checksum(entry, entry[5]) == 0)
            *entry32 = phys;

        if (*entry32 && *entry64)
            return;
    }
}

void linux_find_smbios(uintptr_t *entry32, uintptr_t *entry64) {
    uintptr_t found32 = 0;
    uintptr_t found64 = 0;
    const uint16_t *ebda_segment = linux_phys_to_virt(0x40e);

    if (ebda_segment) {
        uintptr_t ebda = (uintptr_t)(*ebda_segment) << 4;
        if (ebda >= 0x80000 && ebda < 0xa0000)
            scan_smbios_range(ebda, ebda + 1024, &found32, &found64);
    }
    scan_smbios_range(0xf0000, 0x100000, &found32, &found64);

    if (entry32)
        *entry32 = found32;
    if (entry64)
        *entry64 = found64;
}

static bool guid_equal(const struct efi_guid *left,
                       const struct efi_guid *right) {
    return memcmp(left, right, sizeof(*left)) == 0;
}

static void linux_find_efi_tables(void) {
    struct linux_boot_params *params = context.params;
    uint64_t system_table_phys;
    struct efi_system_table64 *system_table;
    struct efi_configuration_table64 *tables;

    if (!params || params->efi_info.loader_signature != 0x34364c45U)
        return;

    system_table_phys = (uint64_t)params->efi_info.system_table |
                        ((uint64_t)params->efi_info.system_table_hi << 32);
    system_table = linux_phys_to_virt(system_table_phys);
    if (!system_table || system_table->table_entry_count > 4096)
        return;
    tables = linux_phys_to_virt(system_table->configuration_table);
    if (!tables)
        return;

    for (uint64_t i = 0; i < system_table->table_entry_count; i++) {
        struct efi_configuration_table64 *table = &tables[i];
        if (!context.acpi_rsdp && guid_equal(&table->guid, &acpi_20_table_guid))
            context.acpi_rsdp = table->table;
        else if (!context.acpi_rsdp &&
                 guid_equal(&table->guid, &acpi_table_guid))
            context.acpi_rsdp = table->table;
        else if (!context.smbios64 &&
                 guid_equal(&table->guid, &smbios3_table_guid))
            context.smbios64 = table->table;
        else if (!context.smbios32 &&
                 guid_equal(&table->guid, &smbios_table_guid))
            context.smbios32 = table->table;
    }
}

void linux_init(uintptr_t boot_params_phys) {
    memset(&context, 0, sizeof(context));
    context.params_phys = boot_params_phys;
    context.params = linux_phys_to_virt(boot_params_phys);
    if (context.params)
        context.acpi_rsdp = context.params->acpi_rsdp_addr;
    context.boot_time = linux_read_boot_time();
    linux_find_smbios(&context.smbios32, &context.smbios64);
    linux_find_efi_tables();
}

linux_boot_context_t *linux_context(void) { return &context; }
