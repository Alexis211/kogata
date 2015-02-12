#pragma once

#include <vfs.h>

// The nullfs accepts several flags at its creation :
//  - c : allow creation of files as ram files
//	- d : allow deletion of arbitrary nodes
//	- m : allow move operation

void register_nullfs_driver(); 

// These calls can be done on the fs_t corresponding to a nullfs
// Of course they fail if the fs is not actually a nullfs.
bool nullfs_add_node(fs_t *f, const char* name, fs_node_ptr data, fs_node_ops_t *ops);
bool nullfs_add_ram_file(fs_t *f, const char* name, char* data, size_t init_sz, bool copy, int ok_modes);

/* vim: set ts=4 sw=4 tw=0 noet :*/
