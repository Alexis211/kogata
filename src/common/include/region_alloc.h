#pragma once

// Virtual memory region allocator

// This is entirely thread-safe

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct region_info;

typedef bool (*map_page_fun_t)(void* addr);	// map a single page (used by region allocator)

typedef struct region_info {
	void* addr;
	size_t size;
	char* type;
} region_info_t;

// rsvd_end : when used for kernel memory region management, a reserved region
// exists between begin (=K_HIGHHALF_ADDR) and the end of kernel static data
// for user processes, use rsvd_end = begin (no reserved region)
void region_allocator_init(void* begin, void* rsvd_end, void* end, map_page_fun_t map);

void* region_alloc(size_t size, char* type);	// returns 0 on error
region_info_t *find_region(void* addr);
void region_free(void* addr);

void dbg_print_region_info();

/* vim: set ts=4 sw=4 tw=0 noet :*/
