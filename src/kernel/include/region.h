#pragma once

#include <region_alloc.h>


// some functions for freeing regions and frames
// region_free_unmap_free : deletes a region and frees all frames that were mapped in it
void region_free_unmap_free(void* addr);
// region_free_unmap : deletes a region and unmaps all frames that were mapped in it, without freeing them
void region_free_unmap(void* addr);

bool alloc_map_single_frame(void* addr);	// used by kernel region allocator

/* vim: set ts=4 sw=4 tw=0 noet :*/
