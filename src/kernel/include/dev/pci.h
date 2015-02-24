#pragma once

#include <sys.h>

#define PCI_MAX_DEVICES 32

// IRQs 9, 10 and 11 are controlled by the PCI driver
// By default all devices are remapped to IRQ 9
// For specific devices other IRQs may be mapped
#define PCI_IRQ_DEFAULT 9
#define PCI_IRQ_STORAGE 10
#define PCI_IRQ_NETWORK 11
#define PCI_IRQ_DISABLE 0xFF

#define PCI_CONFIG_DEVICE_ID 0x02
#define PCI_CONFIG_VENDOR_ID 0x00
#define PCI_CONFIG_CLASS 0x0B
#define PCI_CONFIG_SUBCLASS 0x0A
#define PCI_CONFIG_HEADER_TYPE 0x0E
#define PCI_CONFIG_SECONDARY_BUS 0x19
#define PCI_CONFIG_INTERRUPT_LINE 0x3C

// base classes
#define PCI_BC_STORAGE 0x01
#define PCI_BC_BRIDGE 0x06

// sub-classes
#define PCI_SC_IDE 0x01
#define PCI_SC_PCI_TO_PCI 0x04

typedef void (*pci_irq_handler_t)();

typedef struct {
	uint8_t bus;
	uint8_t slot;
	uint8_t func;

	uint16_t device_id;
	uint16_t vendor_id;
	
	uint8_t base_class;
	uint8_t sub_class;

	uint8_t irq;
	pci_irq_handler_t irq_handler;
} pci_device_t;

extern pci_device_t pci_devices[PCI_MAX_DEVICES];

void pci_setup();	// does scan and sets up IRQs

void pci_set_irq(int dev_id, uint8_t irq);	// irq = 0xFF disables the IRQ for this device

uint32_t pci_read_config_long(int dev_id, uint8_t offset);
uint16_t pci_read_config_word(int dev_id, uint8_t offset);
uint8_t pci_read_config_byte(int dev_id, uint8_t offset);

void pci_write_config_long(int dev_id, uint8_t offset, uint32_t l);
void pci_write_config_word(int dev_id, uint8_t offset, uint16_t w);
void pci_write_config_byte(int dev_id, uint8_t offset, uint8_t b);


/* vim: set ts=4 sw=4 tw=0 noet :*/
