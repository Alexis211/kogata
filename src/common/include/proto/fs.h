#pragma once

#include <stdint.h>
#include <stddef.h>

typedef int fd_t;

#define FT_REGULAR      0 		// no flags = regular file
#define FT_DIR 			(0x01)
#define FT_DEV 			(0x02)
#define FT_BLOCKDEV 	(0x04)
#define FT_CHARDEV 		(0x08)
#define FT_CHANNEL 		(0x10)		// dual-pipe
#define FT_FRAMEBUFFER	(0x20)

// FM_* enum describes modes for opening a file as well as authorized operations in stat_t
// (some flags are used only for open() or only in stat_t.access)
#define FM_READ 	(0x01)
#define FM_WRITE 	(0x02)
#define FM_READDIR 	(0x04)
#define FM_MMAP 	(0x08)
#define FM_CREATE   (0x10)
#define FM_TRUNC    (0x20)
#define FM_APPEND   (0x40)
#define FM_IOCTL	(0x100)
#define FM_BLOCKING	(0x200)
#define FM_DCREATE	(0x1000)		// create file in directory
#define FM_DMOVE	(0x2000)		// move file from directory
#define FM_DDELETE	(0x4000)		// delete file from directory

#define FM_ALL_MODES (0xFFFF)


typedef struct {
	int type;
	int access;
	size_t size;
	// TODO : times & more metadata
} stat_t;

#define DIR_MAX 	128 	// maximum length for a filename

typedef struct {
	char name[DIR_MAX];
	stat_t st;
} dirent_t;


#define IOCTL_BLOCKDEV_GET_BLOCK_SIZE   40
#define IOCTL_BLOCKDEV_GET_BLOCK_COUNT  41


#define SEL_READ	(0x01)
#define	SEL_WRITE	(0x02)
#define SEL_ERROR	(0x04)

typedef struct {
	fd_t fd;
	uint16_t req_flags, got_flags;		// rq_flags : what caller is interested in
} sel_fd_t;

/* vim: set ts=4 sw=4 tw=0 noet :*/
