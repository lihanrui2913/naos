#pragma once

#include <libs/klibc.h>

struct irq_controller;
struct msi_desc_t;

int arch_pci_msi_prepare(struct msi_desc_t *msi_desc, uint32_t index);
void arch_pci_msi_compose_msg(struct msi_desc_t *msi_desc);
void arch_pci_msi_free(struct msi_desc_t *msi_desc);
struct irq_controller *arch_pci_msi_controller(void);
