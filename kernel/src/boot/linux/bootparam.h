#pragma once

#include <libs/klibc.h>

#define LINUX_BOOT_PROTOCOL_VERSION 0x020c
#define LINUX_E820_MAX_ENTRIES 128
#define LINUX_E820_TYPE_RAM 1
#define LINUX_VIDEO_TYPE_VLFB 0x23
#define LINUX_VIDEO_TYPE_EFI 0x70
#define LINUX_VIDEO_CAPABILITY_64BIT_BASE (1U << 1)

struct linux_screen_info {
    uint8_t orig_x;
    uint8_t orig_y;
    uint16_t ext_mem_k;
    uint16_t orig_video_page;
    uint8_t orig_video_mode;
    uint8_t orig_video_cols;
    uint8_t flags;
    uint8_t unused2;
    uint16_t orig_video_ega_bx;
    uint16_t unused3;
    uint8_t orig_video_lines;
    uint8_t orig_video_is_vga;
    uint16_t orig_video_points;
    uint16_t lfb_width;
    uint16_t lfb_height;
    uint16_t lfb_depth;
    uint32_t lfb_base;
    uint32_t lfb_size;
    uint16_t cl_magic;
    uint16_t cl_offset;
    uint16_t lfb_linelength;
    uint8_t red_size;
    uint8_t red_pos;
    uint8_t green_size;
    uint8_t green_pos;
    uint8_t blue_size;
    uint8_t blue_pos;
    uint8_t rsvd_size;
    uint8_t rsvd_pos;
    uint16_t vesapm_seg;
    uint16_t vesapm_off;
    uint16_t pages;
    uint16_t vesa_attributes;
    uint32_t capabilities;
    uint32_t ext_lfb_base;
    uint8_t reserved[2];
} __attribute__((packed));

struct linux_setup_header {
    uint8_t setup_sects;
    uint16_t root_flags;
    uint32_t syssize;
    uint16_t ram_size;
    uint16_t vid_mode;
    uint16_t root_dev;
    uint16_t boot_flag;
    uint16_t jump;
    uint32_t header;
    uint16_t version;
    uint32_t realmode_swtch;
    uint16_t start_sys_seg;
    uint16_t kernel_version;
    uint8_t type_of_loader;
    uint8_t loadflags;
    uint16_t setup_move_size;
    uint32_t code32_start;
    uint32_t ramdisk_image;
    uint32_t ramdisk_size;
    uint32_t bootsect_kludge;
    uint16_t heap_end_ptr;
    uint8_t ext_loader_ver;
    uint8_t ext_loader_type;
    uint32_t cmd_line_ptr;
    uint32_t initrd_addr_max;
    uint32_t kernel_alignment;
    uint8_t relocatable_kernel;
    uint8_t min_alignment;
    uint16_t xloadflags;
    uint32_t cmdline_size;
    uint32_t hardware_subarch;
    uint64_t hardware_subarch_data;
    uint32_t payload_offset;
    uint32_t payload_length;
    uint64_t setup_data;
    uint64_t pref_address;
    uint32_t init_size;
    uint32_t handover_offset;
    uint32_t kernel_info_offset;
} __attribute__((packed));

struct linux_efi_info {
    uint32_t loader_signature;
    uint32_t system_table;
    uint32_t memory_descriptor_size;
    uint32_t memory_descriptor_version;
    uint32_t memory_map;
    uint32_t memory_map_size;
    uint32_t system_table_hi;
    uint32_t memory_map_hi;
} __attribute__((packed));

struct linux_e820_entry {
    uint64_t addr;
    uint64_t size;
    uint32_t type;
} __attribute__((packed));

struct linux_boot_params {
    struct linux_screen_info screen_info; /* 0x000 */
    uint8_t pad_040[0x70 - 0x40];
    uint64_t acpi_rsdp_addr; /* 0x070 */
    uint8_t pad_078[0xc0 - 0x78];
    uint32_t ext_ramdisk_image; /* 0x0c0 */
    uint32_t ext_ramdisk_size;
    uint32_t ext_cmd_line_ptr;
    uint8_t pad_0cc[0x1c0 - 0xcc];
    struct linux_efi_info efi_info; /* 0x1c0 */
    uint32_t alt_mem_k;
    uint32_t scratch;
    uint8_t e820_entries;
    uint8_t eddbuf_entries;
    uint8_t edd_mbr_sig_entries;
    uint8_t keyboard_status;
    uint8_t secure_boot;
    uint8_t pad_1ed[4];
    struct linux_setup_header hdr; /* 0x1f1 */
    uint8_t pad_268[0x2d0 - 0x1f1 - sizeof(struct linux_setup_header)];
    struct linux_e820_entry e820_table[LINUX_E820_MAX_ENTRIES]; /* 0x2d0 */
} __attribute__((packed));

_Static_assert(sizeof(struct linux_screen_info) == 0x40,
               "screen_info layout mismatch");
_Static_assert(offsetof(struct linux_boot_params, acpi_rsdp_addr) == 0x70,
               "boot_params RSDP offset mismatch");
_Static_assert(offsetof(struct linux_boot_params, efi_info) == 0x1c0,
               "boot_params EFI offset mismatch");
_Static_assert(offsetof(struct linux_boot_params, hdr) == 0x1f1,
               "boot_params setup header offset mismatch");
_Static_assert(offsetof(struct linux_boot_params, e820_table) == 0x2d0,
               "boot_params E820 offset mismatch");
