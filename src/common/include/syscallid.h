#pragma once


#define SC_MAX		128		// maximum number of syscalls

#define SC_DBG_PRINT	0
#define SC_EXIT			1
#define SC_YIELD		2

#define	SC_MMAP			10
#define SC_MMAP_FILE	11
#define SC_MCHMAP		12
#define SC_MUNMAP		13

#define SC_CREATE		20
#define SC_DELETE		21
#define SC_MOVE			22
#define SC_STAT			23
#define SC_IOCTL		24

#define SC_OPEN			30
#define SC_CLOSE		31
#define SC_READ			32
#define SC_WRITE		33
#define SC_READDIR		34
#define SC_STAT_OPEN	35		// stat on open file handle
#define SC_GET_MODE		36		// get mode for open file handle

#define SC_MAKE_FS		40
#define SC_FS_ADD_SRC	41
#define SC_RM_FS		42

#define SC_NEW_PROC		50
#define SC_BIND_FS		51		// bind FS to children process
#define SC_PROC_EXEC	52		// execute binary in process

// much more to do


/* vim: set ts=4 sw=4 tw=0 noet :*/
