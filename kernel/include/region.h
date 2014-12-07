#pragma once

// Kernel virtual memory region allocator

// This is entirely thread-safe

#include <sys.h>
#include <paging.h>

// Region types
#define REGION_T_KERNEL_BASE	0x00000001		// base kernel code & data
#define REGION_T_DESCRIPTORS	0x00000002		// contains more region descriptors
#define REGION_T_CORE_HEAP		0x00000100		// used for the core kernel heap
#define REGION_T_KPROC_HEAP		0x00000200		// used for a kernel process' heap
#define REGION_T_KPROC_STACK	0x00000400		// used for a kernel process' heap
#define REGION_T_PROC_KSTACK	0x00000800		// used for a process' kernel heap
#define REGION_T_CACHE			0x00001000		// used for cache
#define REGION_T_HW				0x00002000		// used for hardware access

struct region_info;
typedef void (*page_fault_handler_t)(pagedir_t *pd, struct region_info *r, void* addr);

typedef struct region_info {
	void* addr;
	size_t size;
	uint32_t type;
	page_fault_handler_t pf;
} region_info_t;

void region_allocator_init(void* kernel_data_end);

void* region_alloc(size_t size, uint32_t type, page_fault_handler_t pf);	// returns 0 on error
region_info_t *find_region(void* addr);
void region_free(void* addr);

// some usefull PF handlers
// stack_pf_handler : allocates new frames and panics on access to first page of region (stack overflow)
void stack_pf_handler(pagedir_t *pd, struct region_info *r, void* addr);
// default_allocator_pf_handler : just allocates new frames on page faults
void default_allocator_pf_handler(pagedir_t *pd, struct region_info *r, void* addr);

// some functions for freeing regions and frames
// region_free_unmap_free : deletes a region and frees all frames that were mapped in it
void region_free_unmap_free(void* addr);
// region_free_unmap : deletes a region and unmaps all frames that were mapped in it, without freeing them
void region_free_unmap(void* addr);

void dbg_print_region_stats();

/* vim: set ts=4 sw=4 tw=0 noet :*/
