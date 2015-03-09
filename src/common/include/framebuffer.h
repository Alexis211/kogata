#pragma once

typedef struct {
	uint32_t width, height;
	uint32_t pitch;		// bytes per line
	uint32_t bpp;
} framebuffer_info_t;

typedef struct {
	int mode_number;
	framebuffer_info_t geom;
} fbdev_mode_info_t;

#define IOCTL_FBDEV_GET_MODE_INFO	10
#define IOCTL_FBDEV_SET_MODE		11

#define IOCTL_FB_GET_INFO			12

/* vim: set ts=4 sw=4 tw=0 noet :*/
