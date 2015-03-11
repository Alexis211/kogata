#include <string.h>

#include <idt.h>
#include <nullfs.h>
#include <thread.h>

#include <dev/pckbd.h>

#include <proto/keyboard.h>

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

/* vim: set ts=4 sw=4 tw=0 noet :*/
