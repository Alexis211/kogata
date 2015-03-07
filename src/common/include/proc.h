#pragma once

#include <stddef.h>
#include <stdint.h>

#define PS_LOADING	1
#define PS_RUNNING  2
#define PS_FINISHED	3
#define PS_FAILURE	4	// exception or segfault or stuff
#define PS_KILLED	5

#define FAIL_EXCEPTION		1	// unhandled processor exception
#define FAIL_ZEROPTR		2	// segfault at < 4k
#define FAIL_SEGFAULT		3
#define FAIL_SC_SEGFAULT	4	// failed to validate parameter for system call

typedef struct {
	int pid;
	int status;				// one of PS_*
	int exit_code;		// an error code if state == PS_FAILURE
} proc_status_t;

/* vim: set ts=4 sw=4 tw=0 noet :*/
