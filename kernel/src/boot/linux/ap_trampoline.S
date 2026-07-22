#define AP_BASE 0x8000
#define AP_PHYS(symbol) (AP_BASE + symbol - linux_ap_trampoline_start)

.section .linux_ap_trampoline, "ax"
.code16
.global linux_ap_trampoline_start
linux_ap_trampoline_start:
    cli
    cld
    xorw %ax, %ax
    movw %ax, %ds
    lgdtw AP_PHYS(linux_ap_gdt_ptr)
    movl %cr0, %eax
    orl $1, %eax
    movl %eax, %cr0
    ljmpl $0x08, $AP_PHYS(linux_ap_protected)

.code32
linux_ap_protected:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movl AP_PHYS(linux_ap_cr3), %eax
    movl %eax, %cr3
    movl %cr4, %eax
    orl $(1 << 5), %eax
    movl %eax, %cr4
    movl $0xc0000080, %ecx
    rdmsr
    orl $((1 << 8) | (1 << 11)), %eax
    wrmsr

    /* Normalize the SIPI reset state before enabling paging. */
    movl %cr0, %eax
    andl $0x9ffffff3, %eax
    orl $0x80010023, %eax
    movl %eax, %cr0
    ljmpl $0x18, $AP_PHYS(linux_ap_long_mode)

.code64
linux_ap_long_mode:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movq AP_PHYS(linux_ap_stack), %rsp
    xorq %rbp, %rbp
    movq AP_PHYS(linux_ap_entry), %rax
    jmp *%rax

.balign 8
linux_ap_gdt:
    .quad 0
    .quad 0x00cf9a000000ffff
    .quad 0x00cf92000000ffff
    .quad 0x00af9a000000ffff
linux_ap_gdt_ptr:
    .word linux_ap_gdt_ptr - linux_ap_gdt - 1
    .long AP_PHYS(linux_ap_gdt)

.balign 8
.global linux_ap_cr3
linux_ap_cr3:
    .long 0
.global linux_ap_stack
linux_ap_stack:
    .quad 0
.global linux_ap_entry
linux_ap_entry:
    .quad 0
.global linux_ap_trampoline_end
linux_ap_trampoline_end:
