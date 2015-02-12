#pragma once

#include <vfs.h>

// The nullfs accepts several flags at its creation :
//  - c : allow creation of files as ram files
//	- d : allow deletion of arbitrary nodes
//	- m : allow move operation

typedef struct nullfs nullfs_t;

void register_nullfs_driver(); 

nullfs_t* as_nullfs(fs_t *fs);

bool nullfs_add_node(nullfs_t *f, const char* name, fs_node_ptr data, fs_node_ops_t *ops);
bool nullfs_add_ram_file(nullfs_t *f, const char* name, char* data, size_t init_sz, bool copy, int ok_modes);

/* vim: set ts=4 sw=4 tw=0 noet :*/
