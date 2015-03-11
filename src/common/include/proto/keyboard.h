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
#define KBD_LED_NUMLOCK  	1
#define KBD_LED_CAPSLOCK 	4

/* vim: set ts=4 sw=4 tw=0 noet :*/
