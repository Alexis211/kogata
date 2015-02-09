#pragma once

#include <vfs.h>

#define NULLFS_OPT_CREATE_EN 		1
#define NULLFS_OPT_DELETE_EN 		2

typedef struct nullfs nullfs_t;

typedef struct {
	void* (*open)(void* f, int mode, fs_handle_t *h);
	size_t (*read)(void* fd, size_t offset, size_t len, char* buf);
	size_t (*write)(void* fd, size_t offset, size_t len, const char* buf);
	bool (*stat)(void* f, stat_t *st);
	int (*ioctl)(void* f, int command, void* data);
	void (*close)(void* fd);
	void (*dispose)(void* f);
} nullfs_node_ops_t;

void register_nullfs_driver(); 

nullfs_t* as_nullfs(fs_t *fs);

bool nullfs_add(nullfs_t *f, const char* name, void* data, nullfs_node_ops_t* ops);
bool nullfs_add_ram_file(nullfs_t *f, const char* name, void* data, size_t init_sz, bool copy, int ok_modes);

/* vim: set ts=4 sw=4 tw=0 noet :*/
