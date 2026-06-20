#include <arch/aarch64/drivers/msi.h>

#include <arch/aarch64/drivers/gic.h>
#include <drivers/bus/pci_msi.h>
#include <irq/irq_manager.h>

int arch_pci_msi_prepare(struct msi_desc_t *msi_desc, uint32_t index) {
    uint32_t cpu_id;

    if (!msi_desc || !gic_msi_supported())
        return -ENOTSUP;

    cpu_id = cpu_count ? (index % cpu_count) : 0;
    msi_desc->target_cpu = cpu_id;
    msi_desc->processor = cpu_id;
    msi_desc->pci.msi_attribute.is_64 = true;

    return gic_msi_alloc_irq(cpu_id, &msi_desc->irq_num, &msi_desc->msg);
}

void arch_pci_msi_compose_msg(struct msi_desc_t *msi_desc) {
    if (!msi_desc)
        return;

    gic_route_irq(msi_desc->irq_num, msi_desc->target_cpu);
}

void arch_pci_msi_free(struct msi_desc_t *msi_desc) {
    if (!msi_desc)
        return;

    gic_msi_free_irq(msi_desc->irq_num);
}

struct irq_controller *arch_pci_msi_controller(void) { return &gic_controller; }
