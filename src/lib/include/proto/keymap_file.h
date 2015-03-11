#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
	int normal[128];
	int shift[128];
	int caps[128];
	int mod[128];
	int shiftmod[128];
	bool ralt_is_mod;	// true: right alt = alt-gr
} keymap_t;

/* vim: set ts=4 sw=4 tw=0 noet :*/
