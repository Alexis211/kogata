#pragma once

#include <stdbool.h>
#include <malloc.h>

#include <fs.h> 		// common header

// How to use :
// - when using a filesystem : never call the operations in fs_*_ops_t directly, use
// 		the functions defined bellow
// - when programming a filesystem : don't worry about allocating the fs_handle_t and fs_t,
// 		it is done automatically
// - the three types defined below (filesystem, fs node, file handle) are reference-counters to
//		some data managed by the underlying filesystem. The following types are aliases to void*,
//		but are used to disambiguate the different types of void* : fs_handle_ptr, fs_node_ptr, fs_ptr

typedef void* fs_handle_ptr;
typedef void* fs_node_ptr;
typedef void* fs_ptr;

// Structure defining a handle to an open file

typedef struct {
	size_t (*read)(fs_handle_ptr f, size_t offset, size_t len, char* buf);
	size_t (*write)(fs_handle_ptr f, size_t offset, size_t len, const char* buf);
	bool (*stat)(fs_handle_ptr f, stat_t *st);
	void (*close)(fs_handle_ptr f);
} fs_handle_ops_t;

typedef struct fs_handle {
	// These two field are filled by the VFS's generic open() code
	struct fs *fs;
	int refs;

	// These fields are filled by the FS's specific open() code
	fs_handle_ops_t *ops;
	fs_handle_ptr data;
	int mode;
} fs_handle_t;

// Structure defining a filesystem node
// In the future this is where FS-level cache may be implemented : calls to dispose() are not
// executed immediately when refcount falls to zero but only when we need to free memory

typedef struct {
	bool (*open)(fs_node_ptr n, int mode, fs_handle_t *s); 		// open current node
	bool (*stat)(fs_node_ptr n, stat_t *st);
	bool (*walk)(fs_node_ptr n, const char* file, struct fs_node *n);
	bool (*delete)(fs_node_ptr n);
	bool (*create)(fs_node_ptr n, const char* name, int type);	// create sub-node in directory
	int (*ioctl)(fs_node_ptr n, int command, void* data)
	void (*dispose)(fs_node_ptr n);
} fs_node_ops_t;

typedef struct fs_node {
	// These three fields are filled by the VFS's generic walk() code
	struct fs *fs;
	int refs;
	struct fs_node *parent;
	// These fields are filled by the FS's specific walk() code
	fs_node_ops_t *ops;
	fs_node_ptr data;
} fs_node_t;

// Structure defining a filesystem

typedef struct {
	bool (*add_source)(fs_ptr fs, fs_handle_t* source);
	void (*shutdown)(fs_ptr fs);
} fs_ops_t;

typedef struct fs {
	// Filled by VFS's make_fs()
	int refs;
	// Filled by FS's specific make()
	fs_ops_t *ops;
	fs_ptr data;
	// Filled by both according to what is specified for fs_node_t
	fs_node_t root;
} fs_t;

// Structure defining a filesystem driver

typedef struct {
	bool (*make)(fs_handle_t *source, char* opts, fs_t *d);
	bool (*detect)(fs_handle_t *source);
} fs_driver_ops_t;

// Common functions

void register_fs_driver(const char* name, fs_driver_ops_t *ops);

fs_t* make_fs(const char* driver, fs_handle_t *source, char* opts);
bool fs_add_source(fs_t *fs, fs_handle_t *source);
void ref_fs(fs_t *fs);
void unref_fs(fs_t *fs);

bool fs_delete(fs_t *fs, const char* file);
bool fs_rename(fs_t *fs, const char* from, const char* to);
bool fs_stat(fs_t *fs, const char* file, stat_t *st);
int fs_ioctl(fs_t *fs, const char* file, int command, void* data);

fs_handle_t* fs_open(fs_t *fs, const char* file, int mode);
void ref_file(fs_handle_t *file);
void unref_file(fs_handle_t *file);
size_t file_read(fs_handle_t *f, size_t offset, size_t len, char* buf);
size_t file_write(fs_handle_t *f, size_t offset, size_t len, const char* buf);
int file_get_mode(fs_handle_t *f);

/* vim: set ts=4 sw=4 tw=0 noet :*/
