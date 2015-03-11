#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
	uint16_t scancode;
	uint16_t type;
} kbd_event_t;

#define KBD_EVENT_KEYRELEASE 0
#define KBD_EVENT_KEYPRESS 1

#define IOCTL_KBD_SET_LEDS 10

#define KBD_LED_SCROLLLOCK 	1
#define KBD_LED_NUMLOCK  	2
#define KBD_LED_CAPSLOCK 	4

#define KBD_CODE_ESC 		1
#define KBD_CODE_RETURN		28
#define KBD_CODE_BKSP		14
#define KBD_CODE_UP			200
#define KBD_CODE_DOWN		208
#define KBD_CODE_LEFT		203
#define KBD_CODE_RIGHT		205
#define KBD_CODE_HOME		199
#define KBD_CODE_END		207
#define KBD_CODE_PGUP		201
#define KBD_CODE_PGDOWN		209

#define KBD_CODE_LSHIFT		42
#define KBD_CODE_RSHIFT		54
#define KBD_CODE_CAPSLOCK	58
#define KBD_CODE_LCTRL		29
#define KBD_CODE_RCTRL		157
#define KBD_CODE_LALT		56
#define KBD_CODE_RALT		184
#define KBD_CODE_SUPER		219
#define KBD_CODE_MENU		221
#define KBD_CODE_TAB		15
#define KBD_CODE_INS		210
#define KBD_CODE_DEL		211

#define KBD_CODE_F1			59
#define KBD_CODE_F2			60
#define KBD_CODE_F3			61
#define KBD_CODE_F4			62
#define KBD_CODE_F5			63
#define KBD_CODE_F6			64
#define KBD_CODE_F7			65
#define KBD_CODE_F8			66
#define KBD_CODE_F9			67
#define KBD_CODE_F10		68
#define KBD_CODE_F11		87
#define KBD_CODE_F12		88

/* vim: set ts=4 sw=4 tw=0 noet :*/
