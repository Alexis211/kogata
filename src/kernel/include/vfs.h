#pragma once

#include <stdbool.h>
#include <malloc.h>

typedef struct {
	// TODO
	// (should also be moved to a user-visible header)
} stat_t;

#define FM_READ 	(0x01)
#define FM_WRITE 	(0x02)
#define FM_MMAP 	(0x04)
#define FM_CREATE   (0x10)
#define FM_TRUNC    (0x20)
#define FM_APPEND   (0x40)

// How to use :
// - when using a filesystem : never call the operations in fs_*_ops_t directly, use
// 		the functions defined bellow
// - when programming a filesystem : don't worry about allocating the fs_handle_t and fs_t,
// 		it is done automatically


// Structure defining a handle to an open file

typedef struct {
	size_t (*read)(void* f, size_t offset, size_t len, char* buf);
	size_t (*write)(void* f, size_t offset, size_t len, const char* buf);
	void (*close)(void* f);
} fs_handle_ops_t;

typedef struct fs_handle {
	int refs;
	fs_handle_ops_t *ops;
	void* data;
	int mode;
} fs_handle_t;

// Structure defining a filesystem

typedef struct {
	bool (*open)(void *fs, const char* file, int mode, fs_handle_t *s);
	bool (*delete)(void *fs, const char* file);
	bool (*rename)(void *fs, const char* from, const char* to);
	bool (*stat)(void *fs, const char* file, stat_t *st);
	int (*ioctl)(void *fs, const char* file, int command, void* data);
	bool (*add_source)(void* fs, fs_handle_t* source);
	void (*shutdown)(void *fs);
} fs_ops_t;

typedef struct fs {
	int refs;
	fs_ops_t *ops;
	void* data;
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
