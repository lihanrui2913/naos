#include <settings.h>

#define KERNEL_VIRT_OFFSET 0xffffffff80000000
#define PHYS(symbol) (symbol - KERNEL_VIRT_OFFSET)

.section .linux_boot.text, "ax"
.code32
.global linux_start32
linux_start32:
    cli
    cld
    movl %esi, %ebp

    movl $PHYS(__bss_start), %edi
    movl $PHYS(__bss_end), %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    shrl $2, %ecx
    rep stosl

    movl %ebp, PHYS(linux_boot_params_phys)
    movl $0, PHYS(linux_boot_params_phys) + 4
    call linux_build_tables32
    lgdtw linux_boot_gdt_ptr64

    movl $PHYS(linux_boot_pml4), %eax
    movl %eax, %cr3
    movl %cr4, %eax
    orl $(1 << 5), %eax
    movl %eax, %cr4
    movl $0xc0000080, %ecx
    rdmsr
    orl $((1 << 8) | (1 << 11)), %eax
    wrmsr
    movl %cr0, %eax
    andl $0x9ffffff3, %eax
    orl $0x80010023, %eax
    movl %eax, %cr0
    ljmpl $0x18, $linux_start64_low

linux_build_tables32:
    movl $PHYS(linux_identity_pdpt), %eax
    orl $3, %eax
    movl %eax, PHYS(linux_boot_pml4)
    movl $PHYS(linux_identity_pd), %eax
    orl $3, %eax
    movl %eax, PHYS(linux_identity_pdpt)

    movl $PHYS(linux_hhdm_pdpt), %eax
    orl $3, %eax
    movl %eax, PHYS(linux_boot_pml4) + 256 * 8
    movl $PHYS(linux_kernel_pdpt), %eax
    orl $3, %eax
    movl %eax, PHYS(linux_boot_pml4) + 511 * 8
    movl $PHYS(linux_identity_pd), %eax
    orl $3, %eax
    movl %eax, PHYS(linux_kernel_pdpt) + 510 * 8

    movl $PHYS(linux_identity_pd), %edi
    movl $512, %ecx
    movl $0x83, %eax
1:
    movl %eax, (%edi)
    movl $0, 4(%edi)
    addl $0x200000, %eax
    addl $8, %edi
    loop 1b

    movl $PHYS(linux_hhdm_pdpt), %edi
    movl $PHYS(linux_hhdm_pds), %eax
    movl $512, %ecx
2:
    movl %eax, %edx
    orl $3, %edx
    movl %edx, (%edi)
    movl $0, 4(%edi)
    addl $4096, %eax
    addl $8, %edi
    loop 2b

    movl $PHYS(linux_hhdm_pds), %edi
    movl $262144, %ecx
    movl $0x83, %eax
    xorl %edx, %edx
3:
    movl %eax, (%edi)
    movl %edx, 4(%edi)
    addl $0x200000, %eax
    adcl $0, %edx
    addl $8, %edi
    loop 3b
    ret

.org 0x200
.code64
.global linux_start64
linux_start64:
    cli
    cld
    movq %rsi, %r15

    movl $0xc0000080, %ecx
    rdmsr
    orl $(1 << 11), %eax
    wrmsr
    movq %cr0, %rax
    movl $0x9ffffff3, %edx
    andq %rdx, %rax
    movl $0x80010023, %edx
    orq %rdx, %rax
    movq %rax, %cr0

    movl $PHYS(__bss_start), %edi
    movl $PHYS(__bss_end), %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    shrq $3, %rcx
    rep stosq

    movq %r15, PHYS(linux_boot_params_phys)
    call linux_build_tables64
    lgdtq linux_boot_gdt_ptr64(%rip)
    movq $PHYS(linux_boot_pml4), %rax
    movq %rax, %cr3

linux_start64_low:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    leaq linux_boot_stack_end(%rip), %rsp
    xorq %rbp, %rbp
    movabsq $_start, %rax
    jmp *%rax

linux_build_tables64:
    movq $PHYS(linux_identity_pdpt), %rax
    orq $3, %rax
    movq %rax, PHYS(linux_boot_pml4)
    movq $PHYS(linux_identity_pd), %rax
    orq $3, %rax
    movq %rax, PHYS(linux_identity_pdpt)

    movq $PHYS(linux_hhdm_pdpt), %rax
    orq $3, %rax
    movq %rax, PHYS(linux_boot_pml4) + 256 * 8
    movq $PHYS(linux_kernel_pdpt), %rax
    orq $3, %rax
    movq %rax, PHYS(linux_boot_pml4) + 511 * 8
    movq $PHYS(linux_identity_pd), %rax
    orq $3, %rax
    movq %rax, PHYS(linux_kernel_pdpt) + 510 * 8

    movq $PHYS(linux_identity_pd), %rdi
    movl $512, %ecx
    movq $0x83, %rax
4:
    movq %rax, (%rdi)
    addq $0x200000, %rax
    addq $8, %rdi
    loop 4b

    movq $PHYS(linux_hhdm_pdpt), %rdi
    movq $PHYS(linux_hhdm_pds), %rax
    movl $512, %ecx
5:
    movq %rax, %rdx
    orq $3, %rdx
    movq %rdx, (%rdi)
    addq $4096, %rax
    addq $8, %rdi
    loop 5b

    movq $PHYS(linux_hhdm_pds), %rdi
    movl $262144, %ecx
    movq $0x83, %rax
6:
    movq %rax, (%rdi)
    addq $0x200000, %rax
    addq $8, %rdi
    loop 6b
    ret

.section .linux_boot.data, "aw"
.balign 8
.global linux_boot_gdt
linux_boot_gdt:
    .quad 0
    .quad 0x00cf9a000000ffff
    .quad 0x00cf92000000ffff
    .quad 0x00af9a000000ffff
linux_boot_gdt_ptr64:
    .word linux_boot_gdt_ptr64 - linux_boot_gdt - 1
    .quad linux_boot_gdt

.section .data
.balign 8
.global linux_boot_params_phys
linux_boot_params_phys:
    .quad 0

.section .bss
.balign 4096
.global linux_boot_pml4
linux_boot_pml4:
    .skip 4096
.global linux_identity_pdpt
linux_identity_pdpt:
    .skip 4096
.global linux_identity_pd
linux_identity_pd:
    .skip 4096
.global linux_kernel_pdpt
linux_kernel_pdpt:
    .skip 4096
.global linux_hhdm_pdpt
linux_hhdm_pdpt:
    .skip 4096
.global linux_hhdm_pds
linux_hhdm_pds:
    .skip 512 * 4096
.balign 16
linux_boot_stack:
    .skip STACK_SIZE
linux_boot_stack_end:
