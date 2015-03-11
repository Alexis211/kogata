#pragma once

#include <proto/keymap_file.h>


#define KBD_CHAR		0x01
#define KBD_ALT			0x02
#define KBD_CTRL		0x04
#define KBD_SUPER		0x08
#define KBD_SHIFT		0x10
#define KBD_CAPS		0x20
#define KBD_MOD			0x40

typedef struct {
	union {
		int chr;		// if flags & KBD_CHAR, chr is a character number
		int key;		// if !(flags & KBD_CHAR), key is one of KBD_CODE_* defined in <proto/keyboard.h>
	};
	uint32_t flags;	// one of kbd_*
} key_t;

typedef struct {
	keymap_t km;
	uint32_t status;		// mask of alt/ctrl/super
} keyboard_t;

keyboard_t *init_keyboard();
void free_keyboard(keyboard_t *t);

bool load_keymap(keyboard_t *kb, const char* kmname);

key_t keyboard_press(keyboard_t *t, int scancode);			// what key is pressed?
key_t keyboard_release(keyboard_t *t, int scancode);		// what key is released?



/* vim: set ts=4 sw=4 tw=0 noet :*/
