.section .text
.code16
.org 0x1f1
    .byte 7                       /* setup_sects: 4096-byte setup image */
    .word 0                       /* root_flags */
    .long 0                       /* syssize, patched by mk-linux-bzimage */
    .word 0                       /* ram_size */
    .word 0xffff                  /* vid_mode: use boot loader mode */
    .word 0                       /* root_dev */
    .word 0xaa55                  /* boot_flag */
    .byte 0xeb, 0x66              /* setup header extends through 0x267 */
    .ascii "HdrS"
    .word 0x020c
    .long 0                       /* realmode_swtch */
    .word 0x1000                  /* start_sys_seg */
    .word 0                       /* kernel_version */
    .byte 0                       /* type_of_loader */
    .byte 1                       /* LOADED_HIGH */
    .word 0x8000                  /* setup_move_size */
    .long 0x100000                /* code32_start */
    .long 0, 0, 0                 /* ramdisk fields and bootsect_kludge */
    .word 0xfe00                  /* heap_end_ptr */
    .byte 0, 0                    /* extended loader fields */
    .long 0                       /* cmd_line_ptr */
    .long 0x7fffffff              /* initrd_addr_max */
    .long 0x1000                  /* kernel_alignment */
    .byte 0                       /* non-relocatable */
    .byte 12                      /* minimum alignment */
    .word 1                       /* XLF_KERNEL_64 */
    .long 2047                    /* cmdline_size */
    .long 0                       /* hardware_subarch */
    .quad 0                       /* hardware_subarch_data */
    .long 0                       /* payload_offset */
    .long 0                       /* payload_length, patched */
    .quad 0                       /* setup_data */
    .quad 0x100000                /* pref_address */
    .long 0                       /* init_size, patched */
    .long 0                       /* handover_offset */

.org 0x268
setup_entry:
    cli
1:
    hlt
    jmp 1b

.org 4096

