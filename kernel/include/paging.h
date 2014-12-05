#pragma once

#include <sys.h>
#include <stdbool.h>

struct page_directory;
typedef struct page_directory pagedir_t;


void paging_setup(void* kernel_data_end);

pagedir_t *get_current_pagedir();
pagedir_t *get_kernel_pagedir();

void switch_pagedir(pagedir_t *pd);

// these functions are always relative to the currently mapped page directory
uint32_t pd_get_frame(void* vaddr);		// get physical frame for virtual address
int pd_map_page(void* vaddr, uint32_t frame_id, bool rw);		// returns nonzero on error
void pd_unmap_page(void* vaddr);	// does nothing if page not mapped

pagedir_t *create_pagedir();		// returns zero on error
void delete_pagedir(pagedir_t *pd);

/* vim: set ts=4 sw=4 tw=0 noet :*/
