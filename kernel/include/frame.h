#pragma once

#include <sys.h>

// frame.h : physical memory allocator

void frame_init_allocator(size_t total_ram, void** kernel_data_end);

uint32_t frame_alloc(size_t n);		// allocate n consecutive frames (returns 0 on failure)
void frame_free(uint32_t base, size_t n);

void dbg_print_frame_stats();

/* vim: set ts=4 sw=4 tw=0 noet :*/
