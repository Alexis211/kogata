#pragma once

#include <stdint.h>
#include <stddef.h>

// Kernel memory allocator : one slab allocator for shared memory
// Thread-safe.

void kmalloc_setup();

/* vim: set ts=4 sw=4 tw=0 noet :*/
