#pragma once

// A process is basically :
//  - a page directory and a list of segments mapped in user space
//	- a list of file systems each associated to a name
//	- some threads (currently, only one thread per process supported)

// Notes on memory mapping :
//	- mmap creates an empty zone (zero-initialized)
//	- mmap_file increments the refcount of the file handle
//	- mchmap = change mode on already mapped zone (eg. after loading code)

#include <hashtbl.h>

#include <thread.h>
#include <vfs.h>


// Modes for mmaping regions
#define MM_READ		(0x01)
#define MM_WRITE	(0x02)
#define MM_EXEC		(0x04)

#define USERSTACK_ADDR	0xB8000000
#define USERSTACK_SIZE	0x00020000		// 32 KB

struct process;
typedef struct process process_t;

typedef void* proc_entry_t;

process_t *current_process();

process_t *new_process(process_t *parent);
// void delete_process(process_t *p);	// TODO define semantics for freeing stuff

pagedir_t *proc_pagedir(process_t *p);

bool start_process(process_t *p, proc_entry_t entry);	// maps a region for user stack

bool proc_add_fs(process_t *p, fs_t *fs, const char* name);
fs_t *proc_find_fs(process_t *p, const char* name);

bool mmap(process_t *proc, void* addr, size_t size, int mode);		// create empty zone
bool mmap_file(process_t *proc, fs_handle_t *h, size_t offset, void* addr, size_t size, int mode);
bool mchmap(process_t *proc, void* addr, int mode);
bool munmap(process_t *proc, void* addr);

/* vim: set ts=4 sw=4 tw=0 noet :*/
