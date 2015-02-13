#pragma once

#include <stdbool.h>
#include <malloc.h>
#include <hashtbl.h>

#include <fs.h> 		// common header

// How to use :
// - When using a filesystem : never call the operations in fs_*_ops_t directly, use
// 		the functions defined bellow in section "public functions";
//		Users of the VFS should not manipulate directly fs_node_t items, only fs_t and fs_handle_t
// - When programming a filesystem : don't worry about allocating the fs_handle_t and fs_t,
// 		it is done automatically
// - The three types defined below (filesystem, fs node, file handle) are reference-counters to
//		some data managed by the underlying filesystem. The following types are aliases to void*,
//		but are used to disambiguate the different types of void* : fs_handle_ptr, fs_node_ptr, fs_ptr

typedef void* fs_handle_ptr;
typedef void* fs_node_ptr;
typedef void* fs_ptr;

// usefull forward declarations
struct fs;
struct fs_node;
struct fs_handle;

// -------------------------------------------
// Structure defining a handle to an open file

typedef struct {
	size_t (*read)(fs_handle_ptr f, size_t offset, size_t len, char* buf);
	size_t (*write)(fs_handle_ptr f, size_t offset, size_t len, const char* buf);
	bool (*readdir)(fs_handle_ptr f, dirent_t *d);
	void (*close)(fs_handle_ptr f);
} fs_handle_ops_t;

typedef struct fs_handle {
	// These two field are filled by the VFS's generic open() code
	struct fs *fs;
	struct fs_node *node;
	int refs;

	// These fields are filled by the FS's specific open() code
	fs_handle_ops_t *ops;
	fs_handle_ptr data;
	int mode;
} fs_handle_t;

// -------------------------------------------
// Structure defining a filesystem node
// In the future this is where FS-level cache may be implemented : calls to dispose() are not
// executed immediately when refcount falls to zero but only when we need to free memory
// Remarks :
//  - fs_node_t not to be used in public interface
//  - nodes keep a reference to their parent
//  - a FS node is either in memory (after one call to walk() on its parent), or not in memory
//		it can be in memory only once : if it is in memory, walk() cannot be called on the parent again
//  - delete() is expected to unlink the node from its parent (make it inaccessible) and delete
//		the corresponding data. It is guaranteed that delete() is never called on a node that
//		is currently in use. (different from posix semantics !)
//  - the root node of a filesystem is created when the filesystem is created
//	- dispose() is not called on the root node when a filesystem is shutdown
//	- delete() is not expected to delete recursively : it should fail on a non-empty directory

typedef struct {
	bool (*open)(fs_node_ptr n, int mode, fs_handle_t *s); 		// open current node
	bool (*stat)(fs_node_ptr n, stat_t *st);
	bool (*walk)(fs_node_ptr n, const char* file, struct fs_node *node_d);
	bool (*delete)(fs_node_ptr n, const char* file);
	bool (*move)(fs_node_ptr dir, const char* old_name, struct fs_node *new_parent, const char *new_name);
	bool (*create)(fs_node_ptr n, const char* name, int type);	// create sub-node in directory
	int (*ioctl)(fs_node_ptr n, int command, void* data);
	void (*dispose)(fs_node_ptr n);
} fs_node_ops_t;

typedef struct fs_node {
	// These fields are filled by the VFS's generic walk() code
	struct fs *fs;
	int refs;
	hashtbl_t *children;	// not all children, only those in memory

	struct fs_node *parent;
	char* name;				// name in parent

	// These fields are filled by the FS's specific walk() code
	fs_node_ops_t *ops;
	fs_node_ptr data;
} fs_node_t;

// -------------------------------------------
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

// -------------------------------------------
// Structure defining a filesystem driver

typedef struct {
	bool (*make)(fs_handle_t *source, char* opts, fs_t *d);
	bool (*detect)(fs_handle_t *source);
} fs_driver_ops_t;

// -------------------------------------------
// All functions that return a fs_node_t*, fs_t* or fs_handle_t* return an object
// that will have to be dereferenced when not used anymore.
// (on fs_handle_t*, dereferencing is like closing, only that the actual closing happens
//  when refcount falls to zero)

// Internals

void ref_fs_node(fs_node_t *n);
void unref_fs_node(fs_node_t *n);

fs_node_t* fs_walk_one(fs_node_t* from, const char *file);
fs_node_t* fs_walk_path(fs_node_t* from, const char *p);
fs_node_t* fs_walk_path_except_last(fs_node_t* from, const char *p, char* last_file_buf);

// Public functions

void register_fs_driver(const char* name, fs_driver_ops_t *ops);

fs_t* make_fs(const char* driver, fs_handle_t *source, char* opts);
bool fs_add_source(fs_t *fs, fs_handle_t *source);
void ref_fs(fs_t *fs);
void unref_fs(fs_t *fs);

bool fs_create(fs_t *fs, const char* file, int type);
bool fs_delete(fs_t *fs, const char* file);
bool fs_move(fs_t *fs, const char* from, const char* to);
bool fs_stat(fs_t *fs, const char* file, stat_t *st);
int fs_ioctl(fs_t *fs, const char* file, int command, void* data);

fs_handle_t* fs_open(fs_t *fs, const char* file, int mode);
void ref_file(fs_handle_t *file);
void unref_file(fs_handle_t *file);
int file_get_mode(fs_handle_t *f);
size_t file_read(fs_handle_t *f, size_t offset, size_t len, char* buf);
size_t file_write(fs_handle_t *f, size_t offset, size_t len, const char* buf);
bool file_stat(fs_handle_t *f, stat_t *st);
bool file_readdir(fs_handle_t *f, dirent_t *d);

/* vim: set ts=4 sw=4 tw=0 noet :*/
