#pragma once


#define SC_MAX		128		// maximum number of syscalls

#define SC_DBG_PRINT	0		// args: msg, msg_strlen
#define SC_EXIT			1		// args: code
#define SC_YIELD		2		// args: ()

#define	SC_MMAP			10		// args: addr, size, mode
#define SC_MMAP_FILE	11		// args: handle, offset, addr, size, mode
#define SC_MCHMAP		12		// args: addr, new_mode
#define SC_MUNMAP		13		// args: addr

#define SC_CREATE		20		// args: file, file_strlen, type
#define SC_DELETE		21		// args: file, file_strlen
#define SC_MOVE			22		// args: old_file, old_file_strlen, new_file, new_file_strlen
#define SC_STAT			23		// args: file, file_strlen, out stat_t* data
#define SC_IOCTL		24		// args: file, file_strlen, command, out void* data

#define SC_OPEN			30		// args: file, file_strlen, mode
#define SC_CLOSE		31		// args: fd
#define SC_READ			32		// args: fd, offset, size, out char* data
#define SC_WRITE		33		// args: fd, offset, size, data
#define SC_READDIR		34		// args: fd, out dirent_t *data
#define SC_STAT_OPEN	35		// args: fd, out stat_t *data -- stat on open file handle
#define SC_IOCTL_OPEN	36		// args: fd, command, out void* data
#define SC_GET_MODE		37		// args: fd -- get mode for open file handle

#define SC_MAKE_FS		40
#define SC_FS_ADD_SRC	41
#define SC_RM_FS		42

#define SC_NEW_PROC		50
#define SC_BIND_FS		51		// bind FS to children process
#define SC_PROC_EXEC	52		// execute binary in process

// much more to do


/* vim: set ts=4 sw=4 tw=0 noet :*/
