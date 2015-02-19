#pragma once

// User virtual memory region allocator

// This is entirely thread-safe

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <syscall.h>

#define PAGE_SIZE		0x1000
#define PAGE_MASK		0xFFFFF000
#define PAGE_ALIGN_DOWN(x)	(((size_t)x) & PAGE_MASK) 
#define PAGE_ALIGN_UP(x)  	((((size_t)x)&(~PAGE_MASK)) == 0 ? ((size_t)x) : (((size_t)x) & PAGE_MASK) + PAGE_SIZE)

struct region_info;

typedef struct region_info {
	void* addr;
	size_t size;
	char* type;
} region_info_t;

void region_allocator_init(void* begin, void* end);

void* region_alloc(size_t size, char* type);	// returns 0 on error
region_info_t *find_region(void* addr);
void region_free(void* addr);

void dbg_print_region_info();

/* vim: set ts=4 sw=4 tw=0 noet :*/
