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
#include <btree.h>

#include <thread.h>
#include <vfs.h>

#include <mmap.h>

#define USERSTACK_ADDR	0xB8000000
#define USERSTACK_SIZE	0x00020000		// 32 KB

typedef struct process process_t;

typedef struct user_region {
	process_t *proc;

	void* addr;
	size_t size;

	int mode;

	fs_handle_t *file;		// null if not mmaped-file
	size_t file_offset;

	struct user_region *next;
} user_region_t;

typedef struct process {
	pagedir_t *pd;
	struct user_region *regions;
	btree_t *regions_idx;

	hashtbl_t *filesystems;
	hashtbl_t *files;
	int next_fd;

	thread_t *thread;
	uint64_t last_ran;

	int pid;
	struct process *parent;
} process_t;

typedef void* proc_entry_t;

process_t *current_process();

process_t *new_process(process_t *parent);
// void delete_process(process_t *p);	// TODO define semantics for freeing stuff

bool start_process(process_t *p, proc_entry_t entry);	// maps a region for user stack

bool proc_add_fs(process_t *p, fs_t *fs, const char* name);
fs_t *proc_find_fs(process_t *p, const char* name);
void proc_rm_fs(process_t *p, const char* name);
int proc_add_fd(process_t *p, fs_handle_t *f);		// on error returns 0, nonzero other<ise
fs_handle_t *proc_read_fd(process_t *p, int fd);
void proc_close_fd(process_t *p, int fd);

bool mmap(process_t *proc, void* addr, size_t size, int mode);		// create empty zone
bool mmap_file(process_t *proc, fs_handle_t *h, size_t offset, void* addr, size_t size, int mode);
bool mchmap(process_t *proc, void* addr, int mode);
bool munmap(process_t *proc, void* addr);

// for syscalls : check that process is authorized to do that
// (if not, process exits with a segfault)
void probe_for_read(const void* addr, size_t len);
void probe_for_write(const void* addr, size_t len);

/* vim: set ts=4 sw=4 tw=0 noet :*/
