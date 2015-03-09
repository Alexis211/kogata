#include <string.h>
#include <malloc.h>
#include <thread.h>
#include <nullfs.h>
#include <printf.h>

#include <dev/pci.h>
#include <dev/pciide.h>

static void ide_register_device(ide_controller_t *c, uint8_t device, fs_t *iofs);

// ================================ //
// HELPER FUNCTIONS FOR PCI IDE I/O //
// ================================ //

static void ide_write(ide_controller_t *c, uint8_t channel, uint8_t reg, uint8_t data) {
	if (reg > 0x07 && reg < 0x0C) {
		ide_write(c, channel, ATA_REG_CONTROL, 0x80 | c->channels[channel].nIEN);
	}
	if (reg < 0x08) {
		outb(c->channels[channel].base  + reg - 0x00, data);
	} else if (reg < 0x0C) {
		outb(c->channels[channel].base  + reg - 0x06, data);
	} else if (reg < 0x0E) {
		outb(c->channels[channel].ctrl  + reg - 0x0A, data);
	} else if (reg < 0x16) {
		outb(c->channels[channel].bmide + reg - 0x0E, data);
	}
	if (reg > 0x07 && reg < 0x0C) {
		ide_write(c, channel, ATA_REG_CONTROL, c->channels[channel].nIEN);
	}
}

static uint8_t ide_read(ide_controller_t *c, uint8_t channel, uint8_t reg) {
	uint8_t result = 0;
	if (reg > 0x07 && reg < 0x0C) {
		ide_write(c, channel, ATA_REG_CONTROL, 0x80 | c->channels[channel].nIEN);
	}
	if (reg < 0x08) {
		result = inb(c->channels[channel].base + reg - 0x00);
	} else if (reg < 0x0C) {
		result = inb(c->channels[channel].base  + reg - 0x06);
	} else if (reg < 0x0E) {
		result = inb(c->channels[channel].ctrl  + reg - 0x0A);
	} else if (reg < 0x16) {
		result = inb(c->channels[channel].bmide + reg - 0x0E);
	}
	if (reg > 0x07 && reg < 0x0C) {
		ide_write(c, channel, ATA_REG_CONTROL, c->channels[channel].nIEN);
	}
	return result;
}

static void ide_read_buffer(ide_controller_t *c, uint8_t channel, uint8_t reg, void* buffer, size_t quads) {
	if (reg > 0x07 && reg < 0x0C) {
		ide_write(c, channel, ATA_REG_CONTROL, 0x80 | c->channels[channel].nIEN);
	}
	if (reg < 0x08) {
		insl(c->channels[channel].base  + reg - 0x00, buffer, quads);
	} else if (reg < 0x0C) {
		insl(c->channels[channel].base  + reg - 0x06, buffer, quads);
	} else if (reg < 0x0E) {
		insl(c->channels[channel].ctrl  + reg - 0x0A, buffer, quads);
	} else if (reg < 0x16) {
		insl(c->channels[channel].bmide + reg - 0x0E, buffer, quads);
	}
	if (reg > 0x07 && reg < 0x0C) {
		ide_write(c, channel, ATA_REG_CONTROL, c->channels[channel].nIEN);
	}
}

static int ide_polling(ide_controller_t *c, uint8_t channel, bool advanced_check) {
	// (I) Delay 400 nanosecond for BSY to be set:
	for(int i = 0; i < 4; i++)
		ide_read(c, channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns; loop four times.

	// (II) Wait for BSY to be cleared:
	while (ide_read(c, channel, ATA_REG_STATUS) & ATA_SR_BSY);

	if (advanced_check) {
		uint8_t state = ide_read(c, channel, ATA_REG_STATUS); // Read Status Register.

		// (III) Check For Errors:
		if (state & ATA_SR_ERR)
			return 2; // Error.

		// (IV) Check If Device fault:
		if (state & ATA_SR_DF)
			return 1; // Device Fault.

		// (V) Check DRQ:
		// BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
		if ((state & ATA_SR_DRQ) == 0)
			return 3; // DRQ should be set
	}
	return 0; // ok
}

static uint8_t ide_print_error(ide_controller_t *c, int drive, uint8_t err) {
	if (err == 0)
		return err;

	dbg_printf("IDE: ");
	if (err == 1) {
		dbg_printf("- Device Fault\n     ");
		err = 19;
	} else if (err == 2) {
		uint8_t st = ide_read(c, c->devices[drive].channel, ATA_REG_ERROR);
		if (st & ATA_ER_AMNF) {
			dbg_printf("- No Address Mark Found\n     ");
			err = 7;
		}
		if (st & ATA_ER_TK0NF) {
			dbg_printf("- No Media or Media Error\n     ");
			err = 3;
		}
		if (st & ATA_ER_ABRT) {
			dbg_printf("- Command Aborted\n     ");
			err = 20;
		}
		if (st & ATA_ER_MCR) {
			dbg_printf("- No Media or Media Error\n     ");
			err = 3;
		}
		if (st & ATA_ER_IDNF) {
			dbg_printf("- ID mark not Found\n     ");
			err = 21;
		}
		if (st & ATA_ER_MC) {
			dbg_printf("- No Media or Media Error\n     ");
			err = 3;
		}
		if (st & ATA_ER_UNC) {
			dbg_printf("- Uncorrectable Data Error\n     ");
			err = 22;
		}
		if (st & ATA_ER_BBK) {
			dbg_printf("- Bad Sectors\n     ");
			err = 13;
		}
	} else  if (err == 3) {
		dbg_printf("- Reads Nothing\n     ");
		err = 23;
	} else if (err == 4) {
		dbg_printf("- Write Protected\n     ");
		err = 8;
	} else if (err == 5) {
		dbg_printf("- Interrupted\n     ");
	}
	dbg_printf("- [%s %s] %s\n",
			(const char *[]){"Primary", "Secondary"}[c->devices[drive].channel],
			(const char *[]){"Master", "Slave"}[c->devices[drive].drive],
			c->devices[drive].model);

	return err;
}

// ============ //
// IRQ HANDLERS //
// ============ //

STATIC_MUTEX(on_pciirq);
STATIC_MUTEX(on_irq14);
STATIC_MUTEX(on_irq15);

static thread_t *wait_irq14 = 0, *wait_irq15 = 0, *wait_pciirq = 0;

void irq14_handler(registers_t *regs) {
	if (wait_irq14) {
		thread_t *t = wait_irq14;
		wait_irq14 = 0;
		resume_on(t);
	}
}

void irq15_handler(registers_t *regs) {
	if (wait_irq15) {
		thread_t *t = wait_irq15;
		wait_irq15 = 0;
		resume_on(t);
	}
}

void pciirq_handler(int pci_id) {
	if (wait_pciirq) {
		thread_t *t = wait_pciirq;
		wait_pciirq = 0;
		resume_on(t);
	}
}

static void ide_prewait_irq(ide_controller_t *c, int channel) {
	int irq = c->channels[channel].irq;
	if (irq == 14) {
		mutex_lock(&on_irq14);
		wait_irq14 = current_thread;
	} else if (irq == 15) {
		mutex_lock(&on_irq15);
		wait_irq15 = current_thread;
	} else {
		mutex_lock(&on_pciirq);
		wait_pciirq = current_thread;
	}
}

static bool ide_wait_irq(ide_controller_t *c, int channel) {
	bool ret = true;

	int st = enter_critical(CL_NOINT);

	int irq = c->channels[channel].irq;
	if (irq == 14) {
		if (wait_irq14) ret = wait_on(current_thread);
		mutex_unlock(&on_irq14);
	} else if (irq == 15) {
		if (wait_irq15) ret = wait_on(current_thread);
		mutex_unlock(&on_irq15);
	} else {
		if (wait_pciirq) ret = wait_on(current_thread);
		mutex_unlock(&on_pciirq);
	}

	exit_critical(st);
	
	return ret;
}

// ===================================== //
// ACTUAL READING AND WRITING OF SECTORS //
// ===================================== //

static uint8_t ide_ata_access(ide_controller_t *c, int direction,
	uint8_t drive, uint32_t lba, uint8_t numsects, void* ptr)
{
	uint32_t channel = c->devices[drive].channel; // Read the Channel.
	uint32_t slavebit = c->devices[drive].drive; // Read the Drive [Master/Slave]
	uint32_t bus = c->channels[channel].base; // Bus Base, like 0x1F0 which is also data port.
	const uint32_t words = 256; // Almost every ATA drive has a sector-size of 512-byte.

	uint16_t cyl;
	uint8_t head, sect, err;

	// (I) Select one from LBA28, LBA48 or CHS;
	uint8_t lba_mode /* 0: CHS, 1:LBA28, 2: LBA48 */;
	uint8_t lba_io[6];
	if (lba >= 0x10000000) { // Sure Drive should support LBA in this case, or you are
		// giving a wrong LBA.
		// LBA48:
		lba_mode  = 2;
		lba_io[0] = (lba & 0x000000FF) >> 0;
		lba_io[1] = (lba & 0x0000FF00) >> 8;
		lba_io[2] = (lba & 0x00FF0000) >> 16;
		lba_io[3] = (lba & 0xFF000000) >> 24;
		lba_io[4] = 0;
		lba_io[5] = 0;
		head      = 0; // Lower 4-bits of HDDEVSEL are not used here.
	} else if (c->devices[drive].cap & 0x200)  { // Drive supports LBA?
		// LBA28:
		lba_mode  = 1;
		lba_io[0] = (lba & 0x00000FF) >> 0;
		lba_io[1] = (lba & 0x000FF00) >> 8;
		lba_io[2] = (lba & 0x0FF0000) >> 16;
		lba_io[3] = 0; // These Registers are not used here.
		lba_io[4] = 0; // These Registers are not used here.
		lba_io[5] = 0; // These Registers are not used here.
		head      = (lba & 0xF000000) >> 24;
	} else {
		// CHS:
		lba_mode  = 0;
		sect      = (lba % 63) + 1;
		cyl       = (lba + 1  - sect) / (16 * 63);
		lba_io[0] = sect;
		lba_io[1] = (cyl >> 0) & 0xFF;
		lba_io[2] = (cyl >> 8) & 0xFF;
		lba_io[3] = 0;
		lba_io[4] = 0;
		lba_io[5] = 0;
		head      = (lba + 1  - sect) % (16 * 63) / (63); // Head number is written to HDDEVSEL lower 4-bits.
	}

	// (II) See if drive supports DMA or not;
	bool dma = false; // We don't support DMA

	// (III) Wait if the drive is busy;
	while (ide_read(c, channel, ATA_REG_STATUS) & ATA_SR_BSY); // Wait if busy. 

	// (IV) Select Drive from the controller;
	if (lba_mode == 0) {
		ide_write(c, channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head); // Drive & CHS.
	} else {
		ide_write(c, channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head); // Drive & LBA
	}

	// (V) Write Parameters;
	if (lba_mode == 2) {
		ide_write(c, channel, ATA_REG_SECCOUNT1,   0);
		ide_write(c, channel, ATA_REG_LBA3,   lba_io[3]);
		ide_write(c, channel, ATA_REG_LBA4,   lba_io[4]);
		ide_write(c, channel, ATA_REG_LBA5,   lba_io[5]);
	}
	ide_write(c, channel, ATA_REG_SECCOUNT0,   numsects);
	ide_write(c, channel, ATA_REG_LBA0,   lba_io[0]);
	ide_write(c, channel, ATA_REG_LBA1,   lba_io[1]);
	ide_write(c, channel, ATA_REG_LBA2,   lba_io[2]);

	// (VI) Select the command and send it;
	// Routine that is followed:
	// If ( DMA & LBA48)   DO_DMA_EXT;
	// If ( DMA & LBA28)   DO_DMA_LBA;
	// If ( DMA & LBA28)   DO_DMA_CHS;
	// If (!DMA & LBA48)   DO_PIO_EXT;
	// If (!DMA & LBA28)   DO_PIO_LBA;
	// If (!DMA & !LBA#)   DO_PIO_CHS;
	uint8_t cmd;
	if (lba_mode == 0 && !dma && direction == 0) cmd = ATA_CMD_READ_PIO;
	if (lba_mode == 1 && !dma && direction == 0) cmd = ATA_CMD_READ_PIO;   
	if (lba_mode == 2 && !dma && direction == 0) cmd = ATA_CMD_READ_PIO_EXT;   
	if (lba_mode == 0 && dma && direction == 0) cmd = ATA_CMD_READ_DMA;
	if (lba_mode == 1 && dma && direction == 0) cmd = ATA_CMD_READ_DMA;
	if (lba_mode == 2 && dma && direction == 0) cmd = ATA_CMD_READ_DMA_EXT;
	if (lba_mode == 0 && !dma && direction == 1) cmd = ATA_CMD_WRITE_PIO;
	if (lba_mode == 1 && !dma && direction == 1) cmd = ATA_CMD_WRITE_PIO;
	if (lba_mode == 2 && !dma && direction == 1) cmd = ATA_CMD_WRITE_PIO_EXT;
	if (lba_mode == 0 && dma && direction == 1) cmd = ATA_CMD_WRITE_DMA;
	if (lba_mode == 1 && dma && direction == 1) cmd = ATA_CMD_WRITE_DMA;
	if (lba_mode == 2 && dma && direction == 1) cmd = ATA_CMD_WRITE_DMA_EXT;
	ide_write(c, channel, ATA_REG_COMMAND, cmd);               // Send the Command.

	if (dma) {
		if (direction == 0) {
			// DMA Read.
			ASSERT(false);
		} else {
			// DMA Write.
			ASSERT(false);
		}
	} else {
		if (direction == 0) {
			// PIO Read.
			for (int i = 0; i < numsects; i++) {
				err = ide_polling(c, channel, true);
				if (err) return err; // Polling, set error and exit if there is.
				insw(bus, ptr, words);
				ptr += (words*2);
			}
		} else {
			// PIO Write.
			for (int i = 0; i < numsects; i++) {
				ide_polling(c, channel, false); // Polling.
				outsw(bus, ptr, words);
				ptr += (words*2);
			}
			ide_write(c, channel, ATA_REG_COMMAND, 
				(char []){ATA_CMD_CACHE_FLUSH,
						  ATA_CMD_CACHE_FLUSH,
						  ATA_CMD_CACHE_FLUSH_EXT}[lba_mode]);
			ide_polling(c, channel, false); // Polling.
		}
	}

	return 0;
}


static uint8_t ide_atapi_read(ide_controller_t *c, uint8_t drive, uint32_t lba,
	uint8_t numsects, void* ptr)
{
	uint32_t channel  = c->devices[drive].channel;
	uint32_t slavebit = c->devices[drive].drive;
	uint32_t bus      = c->channels[channel].base;
	const uint32_t words    = 1024; // Sector Size. ATAPI drives have a sector size of 2048 bytes.
	uint8_t err;


	// Enable IRQs:
	c->channels[channel].nIEN = 0;
	ide_write(c, channel, ATA_REG_CONTROL, 0);

	// (I): Setup SCSI Packet:
	uint8_t atapi_packet[12];
	atapi_packet[ 0] = ATAPI_CMD_READ;
	atapi_packet[ 1] = 0x0;
	atapi_packet[ 2] = (lba >> 24) & 0xFF;
	atapi_packet[ 3] = (lba >> 16) & 0xFF;
	atapi_packet[ 4] = (lba >> 8) & 0xFF;
	atapi_packet[ 5] = (lba >> 0) & 0xFF;
	atapi_packet[ 6] = 0x0;
	atapi_packet[ 7] = 0x0;
	atapi_packet[ 8] = 0x0;
	atapi_packet[ 9] = numsects;
	atapi_packet[10] = 0x0;
	atapi_packet[11] = 0x0;

	// (II): Select the drive:
	ide_write(c, channel, ATA_REG_HDDEVSEL, slavebit << 4);

	// (III): Delay 400 nanoseconds for select to complete:
	for(int i = 0; i < 4; i++)
		ide_read(c, channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns.

	// (IV): Inform the Controller that we use PIO mode:
	ide_write(c, channel, ATA_REG_FEATURES, 0);         // PIO mode.

	// (V): Tell the Controller the size of buffer:
	ide_write(c, channel, ATA_REG_LBA1, (words * 2) & 0xFF);   // Lower Byte of Sector Size.
	ide_write(c, channel, ATA_REG_LBA2, (words * 2) >> 8);   // Upper Byte of Sector Size.

	// (VI): Send the Packet Command:
	ide_write(c, channel, ATA_REG_COMMAND, ATA_CMD_PACKET);      // Send the Command.

	// (VII): Waiting for the driver to finish or return an error code:
	err = ide_polling(c, channel, true);
	if (err) return err;         // Polling and return if error.

	// (VIII): Sending the packet data:
	ide_prewait_irq(c, channel);
	outsw(bus, atapi_packet, 6);

	// (IX): Receiving Data:
	for (int i = 0; i < numsects; i++) {
		
		if (!ide_wait_irq(c, channel)) goto must_terminate;                  // Wait for an IRQ.
		ide_prewait_irq(c, channel);
		err = ide_polling(c, channel, 1);
		if (err) return err;      // Polling and return if error.

		insw(bus, ptr, words);
		ptr += (words * 2);
	}

	// (X): Waiting for an IRQ:
	if (!ide_wait_irq(c, channel)) goto must_terminate;

	// (XI): Waiting for BSY & DRQ to clear:
	while (ide_read(c, channel, ATA_REG_STATUS) & (ATA_SR_BSY | ATA_SR_DRQ));

	return 0;

must_terminate:
	// TODO : end communication with device...
	dbg_printf("TODO (pciide may be stuck)\n");
	return 5;
}

static uint8_t ide_read_sectors(ide_controller_t *c, uint8_t drive,
	uint32_t lba, uint8_t numsects, void* ptr)
{
	// 1: Check if the drive presents:
	if (drive > 3 || !c->devices[drive].present) return 1;      // Drive Not Found!

	// 2: Check if inputs are valid:
	if (((lba + numsects) >= c->devices[drive].size) && (c->devices[drive].type == IDE_ATA))
		return 2;                     // Seeking to invalid position.

	// 3: Read in PIO Mode through Polling & IRQs:
	if (c->devices[drive].type == IDE_ATA) {
		uint8_t err = ide_ata_access(c, ATA_READ, drive, lba, numsects, ptr);
		return ide_print_error(c, drive, err);
	} else if (c->devices[drive].type == IDE_ATAPI) {
		uint8_t err = 0;
		// for (int i = 0; i < numsects; i++)
		//	err = ide_atapi_read(c, drive, lba + i, 1, ptr + (i*2048));
		err = ide_atapi_read(c, drive, lba, numsects, ptr);
		return ide_print_error(c, drive, err);
	} else {
		return 1;
	}
}

static uint8_t ide_write_sectors(ide_controller_t *c, uint8_t drive,
	uint8_t numsects, uint32_t lba, void* ptr)
{
	// 1: Check if the drive presents:
	if (drive > 3 || !c->devices[drive].present)
		return 1;

	// 2: Check if inputs are valid:
	if (((lba + numsects) > c->devices[drive].size) && (c->devices[drive].type == IDE_ATA))
		return 2;   // Seeking to invalid position.

	// 3: Read in PIO Mode through Polling & IRQs:
	if (c->devices[drive].type == IDE_ATA) {
		uint8_t err = ide_ata_access(c, ATA_WRITE, drive, lba, numsects, ptr);
		return ide_print_error(c, drive, err);
	} else if (c->devices[drive].type == IDE_ATAPI) {
		return 4; // Write-Protected.
	} else {
		return 1;
	}
}

// ============== //
// SETUP ROUTINES //
// ============== //

static void ide_init(int pci_id, ide_controller_t *c, fs_t *iofs) {
	pci_devices[pci_id].data = c;
	c->pci_id = pci_id;

	// Set IRQ to PCI_IRQ_STORAGE
	uint8_t prog_if = pci_read_config_byte(pci_id, PCI_CONFIG_PROG_IF);
	if (prog_if == 0x8A || prog_if == 0x80) {
		c->channels[ATA_PRIMARY].irq = IRQ14;
		c->channels[ATA_SECONDARY].irq = IRQ15;
	} else {
		pci_set_irq(pci_id, PCI_IRQ_STORAGE);
		c->channels[ATA_PRIMARY].irq = PCI_IRQ_STORAGE;
		c->channels[ATA_SECONDARY].irq = PCI_IRQ_STORAGE;
	}
	
	// Set IRQ handler
	pci_devices[pci_id].irq_handler = pciirq_handler;

	// Detect IO ports
	uint32_t bar0 = pci_read_config_long(pci_id, PCI_CONFIG_BAR0);
	uint32_t bar1 = pci_read_config_long(pci_id, PCI_CONFIG_BAR1);
	uint32_t bar2 = pci_read_config_long(pci_id, PCI_CONFIG_BAR2);
	uint32_t bar3 = pci_read_config_long(pci_id, PCI_CONFIG_BAR3);
	uint32_t bar4 = pci_read_config_long(pci_id, PCI_CONFIG_BAR4);

	c->channels[ATA_PRIMARY  ].base  = (bar0 ? (bar0 & 0xFFFFFFFC) : 0x1F0);
	c->channels[ATA_PRIMARY  ].ctrl  = (bar1 ? (bar1 & 0xFFFFFFFC) : 0x3F6);
	c->channels[ATA_SECONDARY].base  = (bar2 ? (bar2 & 0xFFFFFFFC) : 0x170);
	c->channels[ATA_SECONDARY].ctrl  = (bar3 ? (bar3 & 0xFFFFFFFC) : 0x376);
	c->channels[ATA_PRIMARY  ].bmide = (bar4 & 0xFFFFFFFC) + 0; // Bus Master IDE
	c->channels[ATA_SECONDARY].bmide = (bar4 & 0xFFFFFFFC) + 8; // Bus Master IDE

	// Disable IRQs:
	ide_write(c, ATA_PRIMARY  , ATA_REG_CONTROL, 2);
	ide_write(c, ATA_SECONDARY, ATA_REG_CONTROL, 2);

	// Detect drives
	for (int i = 0; i < 4; i++) {
		c->devices[i].present = 0;
	}

	int count = 0;
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			uint8_t err = 0, type = IDE_ATA, status;
			union {
				char b[512];
				uint16_t w[256];
				uint32_t l[128];
			} ide_buf;

			// (I) Select Drive:
			ide_write(c, i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4));
			usleep(1000);

			// (II) Send ATA Identify Command:
			ide_write(c, i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
			usleep(1000);

			// (III) Polling:
			if (ide_read(c, i, ATA_REG_STATUS) == 0) continue; // If Status = 0, No Device.

			while(1) {
				status = ide_read(c, i, ATA_REG_STATUS);
				if ((status & ATA_SR_ERR)) { // If Err, Device is not ATA.
					err = 1;
					break;
				}
				if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // Everything is right.
			}

			// (IV) Probe for ATAPI Devices:
			if (err != 0) {
				uint8_t cl = ide_read(c, i, ATA_REG_LBA1);
				uint8_t ch = ide_read(c, i, ATA_REG_LBA2);

				if ((cl == 0x14 && ch == 0xEB) || (cl == 0x69 && ch == 0x96)) {
					type = IDE_ATAPI;
				} else {
					continue; // Unknown Type (may not be a device).
				}

				ide_write(c, i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
				usleep(1000);
			}

			// (V) Read Identification Space of the Device:
			ide_read_buffer(c, i, ATA_REG_DATA, ide_buf.b, 128);

			// (VI) Read Device Parameters:
			c->devices[count].present      = 1;
			c->devices[count].type         = type;
			c->devices[count].channel      = i;
			c->devices[count].drive        = j;
			c->devices[count].sig          = ide_buf.w[ATA_IDENT_DEVICETYPE / 2];
			c->devices[count].cap          = ide_buf.w[ATA_IDENT_CAPABILITIES / 2];
			c->devices[count].cmdset       = ide_buf.l[ATA_IDENT_COMMANDSETS / 4];

			// (VII) Get Size:
			if (c->devices[count].cmdset & (1 << 26)) {
				// Device uses 48-Bit Addressing:
				c->devices[count].size   = ide_buf.l[ATA_IDENT_MAX_LBA_EXT / 4];
			} else {
				// Device uses CHS or 28-bit Addressing:
				c->devices[count].size   = ide_buf.l[ATA_IDENT_MAX_LBA / 4];
			}

			// (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
			for(int k = 0; k < 40; k += 2) {
				c->devices[count].model[k] = ide_buf.b[ATA_IDENT_MODEL + k + 1];
				c->devices[count].model[k + 1] = ide_buf.b[ATA_IDENT_MODEL + k];
			}
			c->devices[count].model[40] = 0; // Terminate String.

			count++;
		}
	}

	dbg_printf("PCI IDE controller (PCI device %d)\n", pci_id);
	for (int i = 0; i < 4; i++) {
		if (c->devices[i].present) {
			dbg_printf(" Found %s Drive (%d sectors) - %s (irq %d)\n",
					(const char *[]){"ATA", "ATAPI"}[c->devices[i].type],
					c->devices[i].size,
					c->devices[i].model,
					c->channels[c->devices[i].channel].irq);
			ide_register_device(c, i, iofs);
		}
	}
}

void pciide_detect(fs_t *iofs) {
	idt_set_irq_handler(IRQ14, irq14_handler);
	idt_set_irq_handler(IRQ15, irq15_handler);

	for (int i = 0; i < PCI_MAX_DEVICES; i++) {
		if (pci_devices[i].vendor_id != 0xFFFF &&
			pci_devices[i].base_class == PCI_BC_STORAGE &&
			pci_devices[i].sub_class == PCI_SC_IDE)
		{
			ide_controller_t *c = (ide_controller_t*)malloc(sizeof(ide_controller_t));
			if (c == 0) return;

			memset(c, 0, sizeof(ide_controller_t));
			ide_init(i, c, iofs);
		}
	}
}

// ======================== //
// INTERFACING WITH THE VFS //
// ======================== //

static int next_ata_id = 0;
static int next_atapi_id = 0;

typedef struct {
	ide_controller_t *c;
	uint32_t block_size;		// ATA: 512, ATAPI: 2048
	uint8_t device;
	uint8_t type;
} ide_vfs_dev_t;

static bool ide_vfs_open(fs_node_ptr n, int mode);
static bool ide_vfs_stat(fs_node_ptr n, stat_t *st);

static size_t ide_vfs_read(fs_handle_t *f, size_t offset, size_t len, char* buf);
static size_t ide_vfs_write(fs_handle_t *f, size_t offset, size_t len, const char* buf);
static int ide_vfs_ioctl(fs_node_ptr f, int command, void* data);
static void ide_vfs_close(fs_handle_t *f);

static fs_node_ops_t ide_vfs_node_ops = {
	.open = ide_vfs_open,
	.stat = ide_vfs_stat,
	.walk = 0,
	.delete = 0,
	.move = 0,
	.create = 0,
	.dispose = 0,
	.read = ide_vfs_read,
	.write = ide_vfs_write,
	.ioctl = ide_vfs_ioctl,
	.close = ide_vfs_close,
	.readdir = 0,
	.poll = 0,
};

void ide_register_device(ide_controller_t *c, uint8_t device, fs_t *iofs) {
	ide_vfs_dev_t *d = (ide_vfs_dev_t*)malloc(sizeof(ide_vfs_dev_t));
	if (d == 0) return;

	d->c = c;
	d->device = device;
	d->type = c->devices[device].type;
	d->block_size = (d->type == IDE_ATAPI ? 2048 : 512);

	char name[40];

	if (d->type == IDE_ATAPI) {
		snprintf(name, 40, "/atapi%d", next_atapi_id);
		next_atapi_id++;
	} else {
		snprintf(name, 40, "/ata%d", next_ata_id);
		next_ata_id++;
	}

	bool add_ok = nullfs_add_node(iofs, name, d, &ide_vfs_node_ops, 0);
	if (add_ok) {
		dbg_printf(" Registered as %s.\n", name);
	} else {
		free(d);
	}
}

bool ide_vfs_open(fs_node_ptr n, int mode) {
	ide_vfs_dev_t *d = (ide_vfs_dev_t*)n;

	int ok_modes = (FM_READ
				| (d->type == IDE_ATA ? FM_WRITE : 0) 
				| FM_IOCTL);
	if (mode & ~ok_modes) return false;

	return true;
}

bool ide_vfs_stat(fs_node_ptr n, stat_t *st) {
	ide_vfs_dev_t *d = (ide_vfs_dev_t*)n;

	st->type = FT_BLOCKDEV;
	st->access = (d->type == IDE_ATA ? FM_WRITE : 0) | FM_READ | FM_IOCTL;
	st->size = d->c->devices[d->device].size * d->block_size;

	return true;
}

size_t ide_vfs_read(fs_handle_t *h, size_t offset, size_t len, char* buf) {
	ide_vfs_dev_t *d = (ide_vfs_dev_t*)h->node->data;

	if (offset % d->block_size != 0) return 0;
	if (len % d->block_size != 0) return 0;

	uint8_t err = ide_read_sectors(d->c, d->device, offset / d->block_size, len / d->block_size, buf);
	if (err != 0) return 0;

	return len;
}

size_t ide_vfs_write(fs_handle_t *h, size_t offset, size_t len, const char* buf) {
	ide_vfs_dev_t *d = (ide_vfs_dev_t*)h->node->data;

	if (offset % d->block_size != 0) return 0;
	if (len % d->block_size != 0) return 0;
uint8_t err = ide_write_sectors(d->c, d->device,
		offset / d->block_size, len / d->block_size, (char*)buf);
	if (err != 0) return 0;

	return len;
}

int ide_vfs_ioctl(fs_node_ptr f, int command, void* data) {
	ide_vfs_dev_t *d = (ide_vfs_dev_t*)f;

	int ret = 0;

	if (command == IOCTL_BLOCKDEV_GET_BLOCK_SIZE)
		ret = d->block_size;
	if (command == IOCTL_BLOCKDEV_GET_BLOCK_COUNT) 
		ret = d->c->devices[d->device].size;
	
	return ret;
}

void ide_vfs_close(fs_handle_t *h) {
	// nothing to do
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
