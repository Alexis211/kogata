#pragma once

#include <sys.h>
#include <stdbool.h>
#include <idt.h>

// Bits in the error code for page fault
#define PF_PRESENT_BIT		(1<<0)
#define PF_WRITE_BIT		(1<<1)
#define PF_USER_BIT			(1<<2)
#define PF_RSVD_WRITE_BIT	(1<<3)
#define PF_OPFETCH_BIT		(1<<4)

struct page_directory;
typedef struct page_directory pagedir_t;

typedef void (*user_pf_handler_t)(void* handler_data, registers_t *regs, void* addr);

void paging_setup(void* kernel_data_end);

pagedir_t *get_current_pagedir();
pagedir_t *get_kernel_pagedir();

void switch_pagedir(pagedir_t *pd);

// these functions are always relative to the currently mapped page directory
uint32_t pd_get_frame(void* vaddr);		// get physical frame for virtual address
bool pd_map_page(void* vaddr, uint32_t frame_id, bool rw);		// returns true on success, false on failure
void pd_unmap_page(void* vaddr);	// does nothing if page not mapped

// Note on concurrency : we expect that multiple threads will not try to map/unmap
// pages in the same region at the same time. It can nevertheless happen that
// several threads try to map pages that belong to the same 4M-section, and in that
// case both might require the allocation of a new PT at the same location. These
// cases are well-handled (the pagedir_t type contains a mutex used for this)

pagedir_t *create_pagedir(user_pf_handler_t pf, void* pf_handler_data);		// returns zero on error
void delete_pagedir(pagedir_t *pd);

/* vim: set ts=4 sw=4 tw=0 noet :*/
