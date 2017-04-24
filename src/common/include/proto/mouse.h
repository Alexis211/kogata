#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
	int16_t delta_x, delta_y;
	int8_t delta_wheel;
	uint8_t lbtn, rbtn, midbtn;
} mouse_event_t;

/* vim: set sts=0 ts=4 sw=4 tw=0 noet :*/
