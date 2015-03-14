#pragma once

#include <stdbool.h>
#include <malloc.h>

#include <hashtbl.h>
#include <mutex.h>

#include <proto/fs.h>

#include <pager.h>

// How to use :
// - When using a filesystem : never call the operations in fs_*_ops_t directly, use
// 		the functions defined bellow in section "public functions";
//		Users of the VFS should not manipulate directly fs_node_t items, only fs_t and fs_handle_t
// - When programming a filesystem : don't worry about allocating the fs_handle_t and fs_t,
// 		it is done automatically
// - The three types defined below (filesystem, fs node, file handle) are reference-counters to
//		some data managed by the underlying filesystem. The following types are aliases to void*,
//		but are used to disambiguate the different types of void* : fs_handle_ptr, fs_node_ptr, fs_ptr

// Conventions :
// - ioctl returns negative values on error. null or positives are valid return values from underlying
//		driver

// About thread safety :
// - The VFS implements a locking mechanism on FS nodes so that only one operation is executed
//		at the same time on a given node (including: open, stat, walk, delete, move, create, ioctl)
// - The VFS does not implement locks on handles : several simultaneous read/writes are possible, and
//		it is the driver's responsiblity to prevent that if necessary
// - The VFS does not lock nodes that have a handle open to them : it is the FS code's responsibility
//		to refuse some commands if neccessary on a node that is open.
// - The VFS does not implement any locking mechanism on filesystems themselves (e.g. the add_source
//		implementation must have its own locking system)

typedef void* fs_node_ptr;
typedef void* fs_ptr;

// usefull forward declarations
struct fs;
struct fs_node;

typedef struct user_region user_region_t;
typedef struct fs_node_ops fs_node_ops_t;

// -------------------------------------------
// Structure defining a handle to an open file
// This structure is entirely managed by the VFS, the underlying filesystem does not see it
// For IPC structures (sockets, channels), fs is null, but a node object is nevertheless
// created by the IPC code.

typedef struct fs_handle {
	struct fs *fs;
	struct fs_node *node;

	int refs;
	mutex_t lock;

	int mode;

	// TODO : keep a position in the file for sequential-append access and such
	// (as it is this structure is quite useless)
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
//	- move() may be called on a node that is currently in use
//  - the root node of a filesystem is created when the filesystem is created
//	- dispose() is not called on the root node when a filesystem is shutdown
//	- delete() is not expected to delete recursively : it should fail on a non-empty directory

typedef struct fs_node_ops {
	bool (*open)(fs_node_t *n, int mode);
	size_t (*read)(fs_handle_t *h, size_t offset, size_t len, char* buf);
	size_t (*write)(fs_handle_t *h, size_t offset, size_t len, const char* buf);
	bool (*readdir)(fs_handle_t *h, size_t ent_no, dirent_t *d);
	int (*poll)(fs_handle_t *h, void** out_wait_obj);
	void (*close)(fs_handle_t *h);

	bool (*stat)(fs_node_t *n, stat_t *st);
	int (*ioctl)(fs_handle_t *h, int command, void* data);

	bool (*walk)(fs_node_t *n, const char* file, struct fs_node *node_d);
	bool (*delete)(fs_node_t *n, const char* file);
	bool (*move)(fs_node_t *n, const char* old_name, struct fs_node *new_parent, const char *new_name);
	bool (*create)(fs_node_t *n, const char* name, int type);	// create sub-node in directory
	void (*dispose)(fs_node_t *n);
} fs_node_ops_t;

typedef struct fs_node {
	// These fields are filled by the VFS's generic walk() code
	int refs;
	mutex_t lock;
	
	char* name;				// name in parent
	struct fs *fs;
	hashtbl_t *children;	// not all children, only those in memory
	struct fs_node *parent;

	// These fields are filled by the FS's specific walk() code
	fs_node_ops_t *ops;
	fs_node_ptr data;

	pager_t *pager;
} fs_node_t;

// -------------------------------------------
// Structure defining a filesystem

typedef struct {
	bool (*add_source)(fs_ptr fs, fs_handle_t* source, const char* opts);
	void (*shutdown)(fs_ptr fs);
} fs_ops_t;

typedef struct fs {
	// Filled by VFS's make_fs()
	int refs;
	mutex_t lock;

	struct fs *from_fs;
	int ok_modes;
	// Filled by FS's specific make() - all zero in the case of a subfs
	fs_ops_t *ops;
	fs_ptr data;
	// Filled by both according to what is specified for fs_node_t
	fs_node_t *root;
} fs_t;

// -------------------------------------------
// Structure defining a filesystem driver

typedef struct {
	bool (*make)(fs_handle_t *source, const char* opts, fs_t *d);
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

fs_t* make_fs(const char* driver, fs_handle_t *source, const char* opts);
fs_t* fs_subfs(fs_t *fs, const char *root, int ok_modes);
bool fs_add_source(fs_t *fs, fs_handle_t *source, const char* opts);
void ref_fs(fs_t *fs);
void unref_fs(fs_t *fs);

bool fs_create(fs_t *fs, const char* file, int type);
bool fs_delete(fs_t *fs, const char* file);
bool fs_move(fs_t *fs, const char* from, const char* to);
bool fs_stat(fs_t *fs, const char* file, stat_t *st);

fs_handle_t* fs_open(fs_t *fs, const char* file, int mode);
void ref_file(fs_handle_t *file);
void unref_file(fs_handle_t *file);

bool file_stat(fs_handle_t *f, stat_t *st);

size_t file_read(fs_handle_t *f, size_t offset, size_t len, char* buf);
size_t file_write(fs_handle_t *f, size_t offset, size_t len, const char* buf);
int file_ioctl(fs_handle_t *f, int command, void* data);
bool file_readdir(fs_handle_t *f, size_t ent_no, dirent_t *d);
int file_poll(fs_handle_t *f, void** out_wait_obj);	// just polls the file & returns a mask of SEL_* (see <fs.h>)

/* vim: set ts=4 sw=4 tw=0 noet :*/
