#pragma once

// Framebuffer-related data structures

#include <stdint.h>
#include <stddef.h>

#define FB_MM_RGB15		1		// 15 bits per pixel, blue 0-4, green 5-9, red 10-14
#define FB_MM_BGR15		2		// 15 bits per pixel, red 0-4, green 5-9, blue 10-14
#define FB_MM_RGB16		3		// 2 bytes (16 bits) per pixel, blue 0-4, green 5-9, red 10-14
#define FB_MM_BGR16		4		// 2 bytes (16 bits) per pixel, red 0-4, green 5-9, blue 10-14
#define FB_MM_RGB24		5		// 3 bytes (24 bits) per pixel, blue 0-7, green 8-15, red 16-23
#define FB_MM_BGR24		6		// 3 bytes (24 bits) per pixel, red 0-7, green 8-15, blue 16-23
#define FB_MM_RGB32		7		// 4 bytes (32 bits) per pixel, blue 0-7, green 8-15, red 16-23
#define FB_MM_BGR32		8		// 4 bytes (32 bits) per pixel, red 0-7, green 8-15, blue 16-23
#define FB_MM_GREY8		10		// 1 byte (8 bits) per pixel greyscale

typedef struct {
	uint32_t width, height;
	uint32_t pitch;		// bytes per line
	uint32_t bpp;
	uint32_t memory_model;
} fb_info_t;

typedef struct {
	uint32_t x, y;
	uint32_t w, h;
} fb_region_t;


typedef struct {
	int mode_number;
	fb_info_t geom;
} fbdev_mode_info_t;

#define IOCTL_FB_GET_INFO			1

#define IOCTL_FBDEV_GET_MODE_INFO	10
#define IOCTL_FBDEV_SET_MODE		11

/* vim: set ts=4 sw=4 tw=0 noet :*/
