#pragma once

#include <proc.h>

typedef int fd_t;
typedef int pid_t;
typedef struct { fd_t a, b; } fd_pair_t;

#define SC_MAX		128		// maximum number of syscalls

#define SC_DBG_PRINT	0		// args: msg, msg_strlen
#define SC_EXIT			1		// args: code
#define SC_YIELD		2		// args: ()
#define SC_USLEEP		3		// args: usecs
#define SC_NEW_THREAD	4		// args: eip, esp
#define SC_EXIT_THREAD	5		// args: ()

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

#define SC_MK_CHANNEL	40		// args: blocking?, (int, int)*
#define SC_GEN_TOKEN	41		// args: fd, token_t*
#define SC_USE_TOKEN	42		// args: token_t*

#define SC_MAKE_FS		50		// args: sc_make_fs_args_t
#define SC_FS_ADD_SRC	51		// args: fs_name, fs_name_strlen, fd, opts, opts_strlen
#define SC_SUBFS		52		// args: sc_subfs_args_t
#define SC_RM_FS		53		// args: fs_name, fs_name_strlen
// TODO : how do we enumerate filesystems ?

#define SC_NEW_PROC		60		// args: nothing ?
#define SC_BIND_FS		61		// args: pid, new_name, new_name_strlen, fs_name, fs_name_strlen -- bind FS to child process
#define SC_BIND_SUBFS	62		// args: sc_subfs_args_t -- subfs & bind to child process
#define SC_BIND_MAKE_FS	63		// args: sc_make_fs_args_t
#define SC_BIND_FD		64		// args: pid, new_fd, local_fd -- copy a file descriptor to child process
#define SC_PROC_EXEC	65		// args: pid, exec_name, exec_name_strlen -- execute binary in process
#define SC_PROC_STATUS	66		// args: pid, proc_status_t*
#define SC_PROC_KILL	67		// args: pid, proc_status_t* -- inconditionnally kill child process
#define SC_PROC_WAIT	68		// args: pid?, block?, proc_status_t*

#define INVALID_PID 0		// do a wait with this PID to wayt for any child

typedef struct {
	const char* driver;
	size_t driver_strlen;

	const char* fs_name;
	size_t fs_name_strlen;

	fd_t source_fd;

	const char* opts;
	size_t opts_strlen;

	pid_t bind_to_pid;		// zero = bind to current proc
} sc_make_fs_args_t;

typedef struct {
	const char* new_name;
	size_t new_name_strlen;

	const char* from_fs;
	size_t from_fs_strlen;

	const char* root;
	size_t root_strlen;

	int ok_modes;

	pid_t bind_to_pid;		// 0 = bind to current proc
} sc_subfs_args_t;

/* vim: set ts=4 sw=4 tw=0 noet :*/
