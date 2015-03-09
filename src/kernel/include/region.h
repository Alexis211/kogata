#pragma once

// Kernel virtual memory region allocator

// This is entirely thread-safe

#include <sys.h>
#include <paging.h>

struct region_info;

typedef struct region_info {
	void* addr;
	size_t size;
	char* type;
} region_info_t;

void region_allocator_init(void* kernel_data_end);

void* region_alloc(size_t size, char* type);	// returns 0 on error
region_info_t *find_region(void* addr);
void region_free(void* addr);

// some functions for freeing regions and frames
// region_free_unmap_free : deletes a region and frees all frames that were mapped in it
void region_free_unmap_free(void* addr);
// region_free_unmap : deletes a region and unmaps all frames that were mapped in it, without freeing them
void region_free_unmap(void* addr);

void dbg_print_region_info();

/* vim: set ts=4 sw=4 tw=0 noet :*/
