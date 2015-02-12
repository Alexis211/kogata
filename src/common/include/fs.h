#pragma once

#include <stdint.h>
#include <stddef.h>

#define FT_REGULAR      0 		// no flags = regular file
#define FT_DIR 			(0x01)
#define FT_DEV 			(0x02)
#define FT_BLOCKDEV 	(0x04)
#define FT_CHARDEV 		(0x08)
#define FT_FIFO 		(0x10)
// #define FT_SOCKET 		(0x20)		// Not yet! Semantics not defined.	 (TODO)

#define FM_READ 	(0x01)
#define FM_WRITE 	(0x02)
#define FM_READDIR 	(0x04)
#define FM_MMAP 	(0x08)
#define FM_CREATE   (0x10)
#define FM_TRUNC    (0x20)
#define FM_APPEND   (0x40)

typedef struct {
	int type;
	int access;
	size_t size;
	// TODO : times & more metadata
} stat_t;

#define DIR_MAX 	128 	// maximum length for a filename

typedef struct {
	char name[DIR_MAX];
} dirent_t;


/* vim: set ts=4 sw=4 tw=0 noet :*/
