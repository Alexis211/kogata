#pragma once

#include <hashtbl.h>
#include <mutex.h>

#include <paging.h>

#define PAGE_PINNED		0x01
#define PAGE_ACCESSED	0x02
#define PAGE_DIRTY		0x04

typedef struct pager pager_t;
typedef struct user_region user_region_t;
typedef struct fs_node fs_node_t;
typedef struct fs_handle fs_handle_t;

typedef struct {
	size_t (*read)(fs_node_t* data, size_t offset, size_t len, char* ptr);
	size_t (*write)(fs_node_t* data, size_t offset, size_t len, const char* ptr);
	bool (*resize)(fs_node_t* data, size_t new_size);
} vfs_pager_ops_t;

typedef struct {
	void (*page_in)(pager_t *p, size_t offset, size_t len);			// allocates the pages
	void (*page_commit)(pager_t *p, size_t offset, size_t len);		// writes pages back to storage (if applicable)
	void (*page_out)(pager_t *p, size_t offset, size_t len);		// frees the pages (no need to write back)
	void (*page_release)(pager_t *p, size_t offset, size_t len);	// called on delete/resize
	bool (*resize)(pager_t *p, size_t new_size);
} pager_ops_t;

typedef struct pager {
	pager_ops_t *ops;

	union {
		struct {
			bool allow_resize;
		} swap_pager;
		struct {
			fs_node_t* node;
			vfs_pager_ops_t *ops;
		} vfs_pager;
		struct {
			void* phys_offset;
		} device_pager;
	};

	size_t size;

	hashtbl_t *pages;
	mutex_t lock;

	user_region_t *maps;
} pager_t;

pager_t* new_swap_pager(size_t size, bool allow_resize);
pager_t* new_vfs_pager(size_t size, fs_node_t* vfs_node, vfs_pager_ops_t *vfs_ops);
pager_t* new_device_pager(size_t size, void* phys_offset);

void change_device_pager(pager_t *p, size_t new_size, void* new_phys_offset);

void delete_pager(pager_t *p);

bool pager_resize(pager_t *p, size_t newsize);

// implement later, when we implement freeing
// void pager_pin_region(pager_t *pager, size_t offset, size_t len);
// void pager_unpin_region(pager_t *pager, size_t offset, size_t len);

void pager_access(pager_t *p, size_t offset, size_t len, bool accessed, bool dirty);
void pager_commit_dirty(pager_t *p);

int pager_get_frame(pager_t *p, size_t offset);

size_t pager_read(pager_t *p, size_t offset, size_t len, char* buf);
size_t pager_write(pager_t *p, size_t offset, size_t len, const char* buf);

void pager_add_map(pager_t *p, user_region_t *reg);
void pager_rm_map(pager_t *p, user_region_t *reg);

// for use by files that use a pager for caching
size_t fs_read_from_pager(fs_handle_t *f, size_t offset, size_t len, char* buf);
size_t fs_write_to_pager(fs_handle_t *f, size_t offset, size_t len, const char* buf);

/* vim: set ts=4 sw=4 tw=0 noet :*/
