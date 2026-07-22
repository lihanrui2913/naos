OUTPUT_FORMAT(elf64-x86-64)
ENTRY(linux_start32)

KERNEL_VIRT_OFFSET = 0xffffffff80000000;

PHDRS
{
    linux_boot PT_LOAD;
    text PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
}

SECTIONS
{
    . = 0x100000;
    __linux_payload_start = .;
    .linux_boot : {
        KEEP(*(.linux_boot.text))
        KEEP(*(.linux_boot.data))
    } :linux_boot

    . = KERNEL_VIRT_OFFSET + 0x200000;
    __kernel_image_start = .;
    .text : AT(ADDR(.text) - KERNEL_VIRT_OFFSET) {
        _stext = .;
        *(.text .text.*)
        _etext = .;
    } :text

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    .rodata : AT(ADDR(.rodata) - KERNEL_VIRT_OFFSET) {
        *(.rodata .rodata.*)
        KEEP(*(.linux_ap_trampoline))
    } :rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    .data : AT(ADDR(.data) - KERNEL_VIRT_OFFSET) {
        *(.data .data.*)
    } :data

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    .kallsyms : AT(ADDR(.kallsyms) - KERNEL_VIRT_OFFSET) {
        KEEP(*(.kallsyms))
    } :data

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    __bss_start = .;
    .bss (NOLOAD) : AT(ADDR(.bss) - KERNEL_VIRT_OFFSET) {
        *(.bss .bss.*)
        *(COMMON)
    } :data
    __bss_end = .;

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    __kernel_image_end = .;
    __kernel_phys_start = __kernel_image_start - KERNEL_VIRT_OFFSET;
    __kernel_phys_end = __kernel_image_end - KERNEL_VIRT_OFFSET;
    __linux_payload_memory_end = __kernel_phys_end;

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
    }
}
