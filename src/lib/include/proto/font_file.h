#pragma once

#include <stdint.h>
#include <stddef.h>

#define ASCII_BITMAP_FONT_MAGIC 0xD184C274

typedef struct {
	uint32_t magic;
	uint16_t cw, ch;
	uint32_t nchars;
} ascii_bitmap_font_header;

/* vim: set ts=4 sw=4 tw=0 noet :*/
