#pragma once

#include <stdint.h>
#include <stddef.h>

// Kernel memory allocator : one slab allocator for shared memory
// Thread-safe.

// The normal malloc() call will try to free some memory when OOM and will loop
// a few times if it cannot. It may fail though.

void kmalloc_setup();

/* vim: set ts=4 sw=4 tw=0 noet :*/
