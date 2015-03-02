#pragma once

#include <stddef.h>
#include <stdint.h>

#define PS_LOADING	1
#define PS_RUNNING  2
#define PS_DONE		3
#define PS_FAILURE	4	// exception or segfault or stuff
#define PS_KILLED	5

typedef struct {
	int pid;
	int state;				// one of PS_*
	int return_code;		// an error code if state == PS_FAILURE
} proc_status_t;

/* vim: set ts=4 sw=4 tw=0 noet :*/
