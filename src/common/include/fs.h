#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
	size_t size;
	// TODO : times & more metadata
} stat_t;

#define FM_READ 	(0x01)
#define FM_WRITE 	(0x02)
#define FM_MMAP 	(0x04)
#define FM_CREATE   (0x10)
#define FM_TRUNC    (0x20)
#define FM_APPEND   (0x40)

