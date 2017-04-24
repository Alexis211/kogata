#include <string.h>

#include <idt.h>
#include <nullfs.h>
#include <thread.h>

#include <dev/pckbdmouse.h>

#include <proto/keyboard.h>
#include <proto/mouse.h>


// ----------------------------------------------
//				Keyboard
// ----------------------------------------------

#define PCKBD_BUF_LEN 16

kbd_event_t pckbd_buf[PCKBD_BUF_LEN];
int pckbd_buf_used = 0;
bool pckbd_escaped = false;

void irq1_handler(registers_t *regs) {
	uint8_t scancode = inb(0x60);
	if (scancode == 0xE0) {
		pckbd_escaped = true;
	} else {
		kbd_event_t ev;
		if (scancode & 0x80) {
			if (pckbd_escaped) {
				ev.scancode = scancode;
				ev.type = KBD_EVENT_KEYRELEASE;
			} else {
				ev.scancode = scancode & 0x7F;
				ev.type = KBD_EVENT_KEYRELEASE;
			}
		} else {
			if (pckbd_escaped) {
				ev.scancode = scancode | 0x80;
				ev.type = KBD_EVENT_KEYPRESS;
			} else {
				ev.scancode = scancode;
				ev.type = KBD_EVENT_KEYPRESS;
			}
		}

		if (pckbd_buf_used < PCKBD_BUF_LEN) {
			pckbd_buf[pckbd_buf_used++] = ev;

			resume_on(&pckbd_buf);
		}

		pckbd_escaped = false;
	}
}

//  ---- VFS operations

bool pckbd_open(fs_node_t *n, int mode);
size_t pckbd_read(fs_handle_t *h, size_t offset, size_t len, char *buf);
int pckbd_poll(fs_handle_t *h, void** out_wait_obj);
void pckbd_close(fs_handle_t *h);
int pckbd_ioctl(fs_handle_t *h, int command, void* data);
bool pckbd_stat(fs_node_t *n, stat_t *st);

fs_node_ops_t pckbd_ops = {
	.open = pckbd_open,
	.read = pckbd_read,
	.write = 0,
	.readdir = 0,
	.poll = pckbd_poll,
	.close = pckbd_close,
	.ioctl = pckbd_ioctl,
	.stat = pckbd_stat,
	.walk = 0,
	.delete = 0,
	.move = 0,
	.create = 0,
	.dispose = 0,
};

void pckbd_setup(fs_t *iofs) {
	uint8_t a = 0, b = 0;
	while ((a = inb(0x60)) != b) b = a;

	idt_set_irq_handler(IRQ1, &irq1_handler);

	nullfs_add_node(iofs, "/input/pckbd", 0, &pckbd_ops, 0);
}

bool pckbd_open(fs_node_t *n, int mode) {
	return (mode == FM_READ);
}

size_t pckbd_read(fs_handle_t *h, size_t offset, size_t len, char *buf) {
	int ret = 0;

	int st = enter_critical(CL_NOINT);

	while (len - ret >= sizeof(kbd_event_t) && pckbd_buf_used > 0) {
		memcpy(buf + ret, &pckbd_buf[0], sizeof(kbd_event_t));

		for (int i = 1; i < pckbd_buf_used; i++) {
			pckbd_buf[i-1] = pckbd_buf[i];
		}
		pckbd_buf_used--;

		ret += sizeof(kbd_event_t);
	}

	exit_critical(st);

	return ret;
}

int pckbd_poll(fs_handle_t *h, void** out_wait_obj) {
	if (out_wait_obj) *out_wait_obj = &pckbd_buf;

	return (pckbd_buf_used > 0 ? SEL_READ : 0);
}

void pckbd_close(fs_handle_t *h) {
	return;	//  nothing to do
}

int pckbd_ioctl(fs_handle_t *h, int command, void* data) {
	if (command == IOCTL_KBD_SET_LEDS) {
		uint8_t leds = (uint32_t)data & 0x07;

		outb(0x60, leds);

		return 1;
	}
	return 0;
}

bool pckbd_stat(fs_node_t *n, stat_t *st) {
	st->type = FT_CHARDEV;
	st->size = 0;
	st->access = FM_READ;

	return true;
}


// ----------------------------------------------
//				Mouse
// ----------------------------------------------

#define PCMOUSE_BUF_LEN 64

mouse_event_t pcmouse_buf[PCMOUSE_BUF_LEN];
int pcmouse_buf_used = 0;

int mouse_cycle = 0;
int8_t mouse_byte[4];

void irq12_handler(registers_t *regs) {
	if (mouse_cycle == 0) {
		mouse_byte[0] = inb(0x60);
		mouse_cycle = 1;
	} else if (mouse_cycle == 1) {
		mouse_byte[1] = inb(0x60);
		mouse_cycle = 2;
	} else if (mouse_cycle == 2) {
		mouse_byte[2] = inb(0x60);
		mouse_cycle = 0;

		if (mouse_byte[0] & (0x80 | 0x40)) return;	// overflow bit is set
		if (pcmouse_buf_used >= PCMOUSE_BUF_LEN) return;

		mouse_event_t e;
		e.delta_x = mouse_byte[1];
		e.delta_y = mouse_byte[2];
		if (mouse_byte[0] & 0x10) e.delta_x |= 0xFF00;
		if (mouse_byte[0] & 0x20) e.delta_y |= 0xFF00;
		e.delta_wheel = 0;
		e.lbtn = ((mouse_byte[0] & 0x01) != 0);
		e.rbtn = ((mouse_byte[0] & 0x02) != 0);
		e.midbtn = ((mouse_byte[0] & 0x04) != 0);

		pcmouse_buf[pcmouse_buf_used++] = e;
		resume_on(&pcmouse_buf);
	}
}

static inline void mouse_wait(int a_type) {
	size_t _time_out = 100000;
	if (a_type == 0) {
		while(_time_out--) {
			if ((inb(0x64) & 1)==1) return;
		}
	} else {
		while(_time_out--) {
			if ((inb(0x64) & 2)==0) return;
		}
	}
}

static inline void mouse_write(uint8_t a_write) {
  mouse_wait(1);
  outb(0x64, 0xD4);
  mouse_wait(1);
  outb(0x60, a_write);
}

static inline uint8_t mouse_read() {
  mouse_wait(0);
  return inb(0x60);
}

void pcmouse_setup_device() {
	// Enable mouse device
	mouse_wait(1);
	outb(0x64, 0xA8);

	// Enable interrupts
	mouse_wait(1);
	outb(0x64, 0x20);
	mouse_wait(0);
	uint8_t status = (inb(0x60) | 2);
	mouse_wait(1);
	outb(0x64, 0x60);
	mouse_wait(0);
	outb(0x60, status);

	// Tell mouse to use default settings
	mouse_write(0xF6);
	mouse_read();

	// Enable mouse
	mouse_write(0xF4);
	mouse_read();
}

bool pcmouse_open(fs_node_t *n, int mode);
size_t pcmouse_read(fs_handle_t *h, size_t offset, size_t len, char *buf);
int pcmouse_poll(fs_handle_t *h, void** out_wait_obj);
void pcmouse_close(fs_handle_t *h);
int pcmouse_ioctl(fs_handle_t *h, int command, void* data);
bool pcmouse_stat(fs_node_t *n, stat_t *st);

fs_node_ops_t pcmouse_ops = {
	.open = pcmouse_open,
	.read = pcmouse_read,
	.write = 0,
	.readdir = 0,
	.poll = pcmouse_poll,
	.close = pcmouse_close,
	.ioctl = pcmouse_ioctl,
	.stat = pcmouse_stat,
	.walk = 0,
	.delete = 0,
	.move = 0,
	.create = 0,
	.dispose = 0,
};

void pcmouse_setup(fs_t *iofs) {
	pcmouse_setup_device();

	idt_set_irq_handler(IRQ12, &irq12_handler);

	nullfs_add_node(iofs, "/input/pcmouse", 0, &pcmouse_ops, 0);
}

bool pcmouse_open(fs_node_t *n, int mode) {
	return (mode == FM_READ);
}

size_t pcmouse_read(fs_handle_t *h, size_t offset, size_t len, char *buf) {
	int ret = 0;

	int st = enter_critical(CL_NOINT);

	while (len - ret >= sizeof(mouse_event_t) && pcmouse_buf_used > 0) {
		memcpy(buf + ret, &pcmouse_buf[0], sizeof(mouse_event_t));

		for (int i = 1; i < pcmouse_buf_used; i++) {
			pcmouse_buf[i-1] = pcmouse_buf[i];
		}
		pcmouse_buf_used--;

		ret += sizeof(mouse_event_t);
	}

	exit_critical(st);

	return ret;
}

int pcmouse_poll(fs_handle_t *h, void** out_wait_obj) {
	if (out_wait_obj) *out_wait_obj = &pcmouse_buf;

	return (pcmouse_buf_used > 0 ? SEL_READ : 0);
}

void pcmouse_close(fs_handle_t *h) {
	return;	//  nothing to do
}

int pcmouse_ioctl(fs_handle_t *h, int command, void* data) {
	// No ioctls implemented...
	return 0;
}

bool pcmouse_stat(fs_node_t *n, stat_t *st) {
	st->type = FT_CHARDEV;
	st->size = 0;
	st->access = FM_READ;

	return true;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
