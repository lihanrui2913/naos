#include <arch/arch.h>
#include <drivers/bus/pci_msi.h>
#include <irq/irq_manager.h>

static inline uint32_t x86_msi_message_address(uint32_t processor) {
    return 0xFEE00000UL | ((processor & 0xFF) << 12);
}

static inline uint32_t x86_msi_message_data(uint16_t vector, bool edge_trigger,
                                            bool assert) {
    return ((uint32_t)(vector & 0xFF) | (edge_trigger ? 0 : (1U << 15)) |
            (edge_trigger ? 0 : (assert ? (1U << 14) : 0)));
}

extern uint32_t cpuid_to_lapicid[MAX_CPU_NUM];

int arch_pci_msi_prepare(struct msi_desc_t *msi_desc, uint32_t index) {
    uint32_t usable = 0;

    if (!msi_desc)
        return -EINVAL;

    for (uint32_t cpu = 0; cpu < cpu_count; cpu++) {
        uint32_t lapic_id = cpuid_to_lapicid[cpu];

        if (!x2apic_mode && lapic_id > 0xFF)
            continue;
        if (usable++ != index)
            continue;

        int irq_num = irq_allocate_irqnum();
        if (irq_num < 0 || irq_num >= ARCH_MAX_IRQ_NUM)
            return -ENOSPC;

        msi_desc->irq_num = (uint16_t)irq_num;
        msi_desc->target_cpu = cpu;
        msi_desc->processor = lapic_id;
        msi_desc->pci.msi_attribute.is_64 = true;
        return 0;
    }

    return -ENOSYS;
}

void arch_pci_msi_compose_msg(struct msi_desc_t *msi_desc) {
    if (!msi_desc)
        return;

    msi_desc->msg.address_hi =
        x2apic_mode ? (msi_desc->processor & 0xFFFFFF00) : 0;
    msi_desc->msg.address_lo = x86_msi_message_address(msi_desc->processor);
    msi_desc->msg.data = x86_msi_message_data(
        msi_desc->irq_num, msi_desc->edge_trigger, msi_desc->assert);
    msi_desc->msg.vector_control = 0;
}

void arch_pci_msi_free(struct msi_desc_t *msi_desc) {
    if (!msi_desc)
        return;

    irq_deallocate_irqnum(msi_desc->irq_num);
}

struct irq_controller *arch_pci_msi_controller(void) {
    return &apic_controller;
}
