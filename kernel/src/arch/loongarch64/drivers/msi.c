#include <arch/loongarch64/drivers/msi.h>

#include <drivers/bus/pci_msi.h>

int arch_pci_msi_prepare(struct msi_desc_t *msi_desc, uint32_t index) {
    (void)msi_desc;
    (void)index;
    return -ENOTSUP;
}

void arch_pci_msi_compose_msg(struct msi_desc_t *msi_desc) { (void)msi_desc; }

void arch_pci_msi_free(struct msi_desc_t *msi_desc) { (void)msi_desc; }

struct irq_controller *arch_pci_msi_controller(void) { return NULL; }
