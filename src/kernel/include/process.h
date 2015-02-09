#pragma once

// Things described in this file are essentially a public interface
// All implementation details are hidden in process.c

// A process is a recipient for user code, as well as for mounting File Systems,
// which allow access to features of the system.

#include <thread.h>

#include <hashtbl.h>


struct process;
typedef struct process process_t;

process_t *new_process(entry_t entry, void* data);

bool mmap(process_t *proc, void* addr, size_t size, int type);

/* vim: set ts=4 sw=4 tw=0 noet :*/
