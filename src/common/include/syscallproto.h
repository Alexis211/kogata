#pragma once

#include <proc.h>

#define SC_MAX		128		// maximum number of syscalls

#define SC_DBG_PRINT	0		// args: msg, msg_strlen
#define SC_EXIT			1		// args: code
#define SC_YIELD		2		// args: ()
#define SC_USLEEP		3		// args: usecs

#define	SC_MMAP			10		// args: addr, size, mode
#define SC_MMAP_FILE	11		// args: handle, offset, addr, size, mode
#define SC_MCHMAP		12		// args: addr, new_mode
#define SC_MUNMAP		13		// args: addr

#define SC_CREATE		20		// args: file, file_strlen, type
#define SC_DELETE		21		// args: file, file_strlen
#define SC_MOVE			22		// args: old_file, old_file_strlen, new_file, new_file_strlen
#define SC_STAT			23		// args: file, file_strlen, out stat_t* data

#define SC_OPEN			30		// args: file, file_strlen, mode
#define SC_CLOSE		31		// args: fd
#define SC_READ			32		// args: fd, offset, size, out char* data
#define SC_WRITE		33		// args: fd, offset, size, data
#define SC_READDIR		34		// args: fd, ent_no, out dirent_t *data
#define SC_STAT_OPEN	35		// args: fd, out stat_t *data -- stat on open file handle
#define SC_IOCTL		36		// args: fd, command, out void* data
#define SC_GET_MODE		37		// args: fd -- get mode for open file handle

#define SC_MAKE_FS		40		// args: sc_make_fs_args_t
#define SC_FS_ADD_SRC	41		// args: fs_name, fs_name_strlen, fd, opts, opts_strlen
#define SC_SUBFS		42		// args: sc_subfs_args_t
#define SC_RM_FS		43		// args: fs_name, fs_name_strlen
// TODO : how do we enumerate filesystems ?

#define SC_NEW_PROC		50		// args: nothing ?
#define SC_BIND_FS		51		// args: pid, new_name, new_name_strlen, fs_name, fs_name_strlen -- bind FS to child process
#define SC_BIND_SUBFS	52		// args: sc_subfs_args_t -- subfs & bind to child process
#define SC_BIND_FD		53		// args: pid, new_fd, local_fd -- copy a file descriptor to child process
#define SC_PROC_EXEC	55		// args: pid, exec_name, exec_name_strlen -- execute binary in process
#define SC_PROC_STATUS	56		// args: pid, proc_status_t*
#define SC_PROC_KILL	57		// args: pid, proc_status_t* -- inconditionnally kill child process
#define SC_PROC_WAIT	58		// args: pid?, block?, proc_status_t*

#define INVALID_PID 0		// do a wait with this PID to wayt for any child

typedef struct {
	const char* driver;
	size_t driver_strlen;

	const char* fs_name;
	size_t fs_name_strlen;

	int source_fd;

	const char* opts;
	size_t opts_strlen;
} sc_make_fs_args_t;

typedef struct {
	const char* new_name;
	size_t new_name_strlen;

	const char* from_fs;
	size_t from_fs_strlen;

	const char* root;
	size_t root_strlen;

	int ok_modes;

	int bind_to_pid;		// used only for SC_BIND_SUBFS
} sc_subfs_args_t;

/* vim: set ts=4 sw=4 tw=0 noet :*/
