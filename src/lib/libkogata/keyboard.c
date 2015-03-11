#include <malloc.h>
#include <string.h>
#include <printf.h>

#include <syscall.h>

#include <proto/keyboard.h>
#include <keyboard.h>

//  ---- Control keys that are not KBD_CHAR-able

int ctrlkeys[] = {
/* 0x00 */	0, KBD_CODE_ESC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KBD_CODE_BKSP, KBD_CODE_TAB,
/* 0x10 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KBD_CODE_RETURN, KBD_CODE_LCTRL, 0, 0,
/* 0x20 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KBD_CODE_LSHIFT, 0, 0, 0, 0, 0,
/* 0x30 */	0, 0, 0, 0, 0, 0, KBD_CODE_RSHIFT, 0, KBD_CODE_LALT, 0,
		KBD_CODE_CAPSLOCK, KBD_CODE_F1, KBD_CODE_F2, KBD_CODE_F3, KBD_CODE_F4, KBD_CODE_F5,
/* 0x40 */	KBD_CODE_F6, KBD_CODE_F7, KBD_CODE_F8, KBD_CODE_F9, KBD_CODE_F10, KBD_CODE_NUMLOCK,
		KBD_CODE_SCRLLOCK, KBD_CODE_KPHOME, KBD_CODE_KPUP, KBD_CODE_KPPGUP, 0, KBD_CODE_KPLEFT,
		KBD_CODE_KP5, KBD_CODE_KPRIGHT, 0, KBD_CODE_KPEND,
/* 0x50 */	KBD_CODE_KPDOWN, KBD_CODE_KPPGDOWN, KBD_CODE_KPINS, KBD_CODE_KPDEL,
		KBD_CODE_SYSREQ, 0, 0, KBD_CODE_F11, KBD_CODE_F12, 0, 0, 0, 0, 0, 0, 0,
/* 0x60 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 0x70 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 0x80 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 0x90 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KBD_CODE_RETURN, KBD_CODE_RCTRL, 0, 0,
/* 0xA0 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 0xB0 */	0, 0, 0, 0, 0, 0, 0, KBD_CODE_PRTSCN, KBD_CODE_RALT, 0, 0, 0, 0, 0, 0, 0,
/* 0xC0 */	0, 0, 0, 0, 0, 0, 0, KBD_CODE_HOME, KBD_CODE_UP, KBD_CODE_PGUP, 0,
		KBD_CODE_LEFT, 0, KBD_CODE_RIGHT, 0, KBD_CODE_END,
/* 0xD0 */	KBD_CODE_DOWN, KBD_CODE_PGDOWN, KBD_CODE_INS, KBD_CODE_DEL, 0, 0, 0, 0, 0,
		0, 0, KBD_CODE_LSUPER, KBD_CODE_RSUPER, KBD_CODE_MENU, 0, 0,
/* 0xE0 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 0xF0 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

//  ---- The code

keyboard_t *init_keyboard() {
	keyboard_t *kb = (keyboard_t*)malloc(sizeof(keyboard_t));
	if (kb == 0) return 0;

	if (!load_keymap(kb, "default")) {
		free(kb);
		return 0;
	}

	return kb;
}

void free_keyboard(keyboard_t *t) {
	free(t);
}

bool load_keymap(keyboard_t *kb, const char* kmname) {
	char buf[128];
	snprintf(buf, 128, "sys:/keymaps/%s.km", kmname);

	fd_t f = open(buf, FM_READ);
	if (f == 0) {
		dbg_printf("Failed to open keymap %s\n", buf);
		return false;
	}

	keymap_t km;
	size_t rd = read(f, 0, sizeof(keymap_t), (char*)&km);

	bool ok = (rd == sizeof(keymap_t));

	if (ok) {
		memcpy(&kb->km, &km, sizeof(keymap_t));
		kb->status = 0;
	}

	close(f);

	return ok;
}

int key_chr(keyboard_t *kb, int k) {
	if (kb->status & KBD_MOD) {
		if ((kb->status & KBD_SHIFT) | (kb->status & KBD_CAPS)) {
			return kb->km.shiftmod[k];
		} else {
			return kb->km.mod[k];
		}
	} else if ((kb->status & KBD_CAPS) && (kb->status & KBD_SHIFT)) {
			return kb->km.normal[k];
	} else if (kb->status & KBD_SHIFT) {
			return kb->km.shift[k];
	} else if (kb->status & KBD_CAPS) {
			return kb->km.caps[k];
	} else {
			return kb->km.normal[k];
	}
}

key_t make_key(keyboard_t *kb, int k) {
	key_t x;
	x.flags = kb->status;

	x.key = 0;
	if (k >= 128) return x;

	x.key = ctrlkeys[k];
	if (x.key != 0) return x;

	x.flags |= KBD_CHAR;
	x.chr = key_chr(kb, k);
	return x;

}

key_t keyboard_press(keyboard_t *kb, int k) {
	if (k == KBD_CODE_LSHIFT || k == KBD_CODE_RSHIFT) {
		kb->status |= KBD_SHIFT;
	} else if (k == KBD_CODE_LCTRL || k == KBD_CODE_RCTRL) {
		kb->status |= KBD_CTRL;
	} else if (k == KBD_CODE_LSUPER || k == KBD_CODE_RSUPER) {
		kb->status |= KBD_SUPER;
	} else if (k == KBD_CODE_LALT || (!kb->km.ralt_is_mod && k == KBD_CODE_RALT)) {
		kb->status |= KBD_ALT;
	} else if (kb->km.ralt_is_mod && k == KBD_CODE_RALT) {
		kb->status |= KBD_MOD;
	} else if (kb->km.ralt_is_mod && k == KBD_CODE_CAPSLOCK) {
		kb->status ^= KBD_CAPS;
	}

	return make_key(kb, k);
}

key_t keyboard_release(keyboard_t *kb, int k) {
	if (k == KBD_CODE_LSHIFT || k == KBD_CODE_RSHIFT) {
		kb->status &= ~KBD_SHIFT;
	} else if (k == KBD_CODE_LCTRL || k == KBD_CODE_RCTRL) {
		kb->status &= ~KBD_CTRL;
	} else if (k == KBD_CODE_LSUPER || k == KBD_CODE_RSUPER) {
		kb->status &= ~KBD_SUPER;
	} else if (k == KBD_CODE_LALT || (!kb->km.ralt_is_mod && k == KBD_CODE_RALT)) {
		kb->status &= ~KBD_ALT;
	} else if (kb->km.ralt_is_mod && k == KBD_CODE_RALT) {
		kb->status &= ~KBD_MOD;
	}

	return make_key(kb, k);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
