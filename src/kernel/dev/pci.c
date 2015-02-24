#include <string.h>

#include <idt.h>

#include <dev/pci.h>

#define BUS(i) pci_devices[i].bus
#define SLOT(i) pci_devices[i].slot
#define FUNC(i) pci_devices[i].func

static void check_bus(uint8_t);
static void pci_irq_default(registers_t*);
static void pci_irq_storage(registers_t*);
static void pci_irq_network(registers_t*);

pci_device_t pci_devices[PCI_MAX_DEVICES];

// ================================= //
// READ-WRITE WITH BUS/SLOT/FUNC IDS //
// ================================= //

static uint32_t read_config_long(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;

	uint32_t address = (uint32_t)((lbus << 16) | (lslot << 11) |
			(lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

	/* write out the address */
	outl(0xCF8, address);
	/* read in the data */
	return inl(0xCFC);
}

static uint16_t read_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
	uint32_t tmp = read_config_long(bus, slot, func, offset);

	return (tmp >> ((offset & 2) * 8)) & 0xFFFF;
}

static uint8_t read_config_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
	uint32_t tmp = read_config_long(bus, slot, func, offset);

	return (tmp >> ((offset & 3) * 8)) & 0xFF;
}

static void write_config_long(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t data) {
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;

	uint32_t address = (uint32_t)((lbus << 16) | (lslot << 11) |
			(lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

	/* write out the address */
	outl(0xCF8, address);
	/* write out the data */
	outl(0xCFC, data);
}

static void write_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t d) {
	uint32_t tmp = read_config_long(bus, slot, func, offset);

	int shift = (offset & 2) * 8;
	uint32_t mask = 0xFFFF << shift;
	tmp = (tmp & ~mask) | (d << shift);

	write_config_long(bus, slot, func, offset, tmp);
}

static void write_config_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint8_t d) {
	uint32_t tmp = read_config_long(bus, slot, func, offset);

	int shift = (offset & 3) * 8;
	uint32_t mask = 0xFF << shift;
	tmp = (tmp & ~mask) | (d << shift);

	write_config_long(bus, slot, func, offset, tmp);
}


// ========================= //
// READ-WRITE WITH DEVICE ID //
// ========================= //

uint32_t pci_read_config_long(int i, uint8_t offset) {
	return read_config_long(BUS(i), SLOT(i), FUNC(i), offset);
}

uint16_t pci_read_config_word(int i, uint8_t offset) {
	return read_config_word(BUS(i), SLOT(i), FUNC(i), offset);
}

uint8_t pci_read_config_byte(int i, uint8_t offset) {
	return read_config_byte(BUS(i), SLOT(i), FUNC(i), offset);
}

void pci_write_config_long(int i, uint8_t offset, uint32_t d) {
	return write_config_long(BUS(i), SLOT(i), FUNC(i), offset, d);
}

void pci_write_config_word(int i, uint8_t offset, uint16_t d) {
	return write_config_word(BUS(i), SLOT(i), FUNC(i), offset, d);
}

void pci_write_config_byte(int i, uint8_t offset, uint8_t d) {
	return write_config_byte(BUS(i), SLOT(i), FUNC(i), offset, d);
}

// ======================= //
// PCI ENUMERATION & SETUP //
// ======================= //

static void check_function(uint8_t bus, uint8_t slot, uint8_t func) {
	static int id = 0;
	if (id >= PCI_MAX_DEVICES) return;

	pci_devices[id].bus = bus;
	pci_devices[id].slot = slot;
	pci_devices[id].func = func;

	pci_devices[id].vendor_id = pci_read_config_word(id, PCI_CONFIG_VENDOR_ID);
	if (pci_devices[id].vendor_id == 0xFFFF) return;

	pci_devices[id].device_id = pci_read_config_word(id, PCI_CONFIG_DEVICE_ID);
	pci_devices[id].base_class = pci_read_config_byte(id, PCI_CONFIG_CLASS);
	pci_devices[id].sub_class = pci_read_config_byte(id, PCI_CONFIG_SUBCLASS);
	pci_set_irq(id, PCI_IRQ_DEFAULT);

	dbg_printf("PCI %d:%d:%d : %x:%x %x %x (irq %d)\n", bus, slot, func,
		pci_devices[id].vendor_id, pci_devices[id].device_id,
		pci_devices[id].base_class, pci_devices[id].sub_class,
		pci_devices[id].irq);

	if (pci_devices[id].base_class == PCI_BC_BRIDGE
		 && pci_devices[id].sub_class == PCI_SC_PCI_TO_PCI)
		check_bus(pci_read_config_byte(id, PCI_CONFIG_SECONDARY_BUS));

	id++;
}

static void check_device(uint8_t bus, uint8_t device) {
	uint8_t function = 0;

	uint16_t vendor_id = read_config_word(bus, device, function, PCI_CONFIG_VENDOR_ID);
	if (vendor_id == 0xFFFF) return;

	check_function(bus, device, function);

	uint8_t header_type = read_config_byte(bus, device, function, PCI_CONFIG_HEADER_TYPE);
	if (header_type & 0x80) {
		for (function = 1; function < 8; function++) {
			vendor_id = read_config_word(bus, device, function, PCI_CONFIG_VENDOR_ID);
			if (vendor_id != 0xFFFF) {
				check_function(bus, device, function);
			}
		}
	}
	
}

void check_bus(uint8_t bus) {
	for (uint8_t device = 0; device < 32; device++) {
		check_device(bus, device);
	}
}

void pci_setup() {
	for (int i = 0; i < PCI_MAX_DEVICES; i++) {
		memset(&pci_devices[i], 0, sizeof(pci_device_t));
		pci_devices[i].vendor_id = 0xFFFF;	// not in use;
	}

	idt_set_irq_handler(PCI_IRQ_DEFAULT, pci_irq_default);
	idt_set_irq_handler(PCI_IRQ_STORAGE, pci_irq_storage);
	idt_set_irq_handler(PCI_IRQ_NETWORK, pci_irq_network);

	if (read_config_byte(0, 0, 0, PCI_CONFIG_HEADER_TYPE) & 0x80) {
		for (uint8_t func = 0; func < 8; func++) {
			if (read_config_word(0, 0, func, PCI_CONFIG_VENDOR_ID) != 0xFFFF)
				check_bus(func);
		}
	} else {
		check_bus(0);
	}
}

// ================ //
// PCI IRQ HANDLING //
// ================ //

void pci_irq_storage(registers_t *r) {
	for (int i = 0; i < PCI_MAX_DEVICES; i++) {
		if (pci_devices[i].irq == PCI_IRQ_STORAGE && pci_devices[i].irq_handler != 0)
			pci_devices[i].irq_handler(i);
	}
}

void pci_irq_network(registers_t *r) {
	for (int i = 0; i < PCI_MAX_DEVICES; i++) {
		if (pci_devices[i].irq == PCI_IRQ_NETWORK && pci_devices[i].irq_handler != 0)
			pci_devices[i].irq_handler(i);
	}
}

void pci_irq_default(registers_t *r) {
	for (int i = 0; i < PCI_MAX_DEVICES; i++) {
		if (pci_devices[i].irq == PCI_IRQ_DEFAULT && pci_devices[i].irq_handler != 0)
			pci_devices[i].irq_handler(i);
	}
}

void pci_set_irq(int dev_id, uint8_t irq) {
	if (dev_id < 0 || dev_id >= PCI_MAX_DEVICES) return;
	if (irq != PCI_IRQ_DEFAULT
		&& irq != PCI_IRQ_STORAGE && irq != PCI_IRQ_NETWORK
		&& irq != PCI_IRQ_DISABLE) return;

	pci_write_config_byte(dev_id, PCI_CONFIG_INTERRUPT_LINE, irq);
	pci_devices[dev_id].irq = pci_read_config_byte(dev_id, PCI_CONFIG_INTERRUPT_LINE);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
