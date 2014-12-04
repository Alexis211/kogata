#pragma once

// Self-contained piece of library : a slab allocator...
// Depends on page_alloc_fun_t and page_free_fun_t : a couple of functions
// that can allocate/free multiples of one page at a time

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <sys.h>

// expected format for the array of slab_type_t given to slab_create :
// an array of slab_type descriptors, with last descriptor full of zeroes
// and with obj_size increasing (strictly) in the array
typedef struct slab_type {
	const char *descr;
	size_t obj_size;
	size_t pages_per_cache;
} slab_type_t;

struct mem_allocator;
typedef struct mem_allocator mem_allocator_t;

typedef void* (*page_alloc_fun_t)(const size_t bytes);
typedef void (*page_free_fun_t)(const void* ptr);

mem_allocator_t* create_slab_allocator(const slab_type_t *types, page_alloc_fun_t af, page_free_fun_t ff);
void destroy_slab_allocator(mem_allocator_t*);

void* slab_alloc(mem_allocator_t* a, const size_t sz);
void slab_free(mem_allocator_t* a, const void* ptr);

/* vim: set ts=4 sw=4 tw=0 noet :*/
