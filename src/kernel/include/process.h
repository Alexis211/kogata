#pragma once

// A process is a recipient for user code, as well as for mounting File Systems,
// which allow access to features of the system.

#include <thread.h>

#include <hashtbl.h>


struct process;
typedef struct process process_t;

process_t *current_process();

process_t *new_process(process_t *parent);
void delete_process(process_t *p);

void start_process(process_t *p, entry_t entry, void* data);

bool proc_add_fs(process_t *p, fs_t *fs, const char* name);
fs_t *proc_find_fs(process_t *p, const char* name);

bool mmap(process_t *proc, void* addr, size_t size, int type);
bool mmap_file(process_t *proc, fs_handle_t *h, void* addr, size_t size, int mode);

/* vim: set ts=4 sw=4 tw=0 noet :*/
