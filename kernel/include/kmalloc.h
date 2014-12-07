#pragma once

#include <stdint.h>
#include <stddef.h>

// Kernel memory allocator : one slab allocator for shared memory
// Thread-safe.

void kmalloc_setup();

void* kmalloc(size_t sz);
void kfree(void* ptr);

/* vim: set ts=4 sw=4 tw=0 noet :*/
