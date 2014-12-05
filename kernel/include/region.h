#pragma once

// Kernel virtual memory region allocator

#include <sys.h>
#include <paging.h>

// Region types
#define REGION_T_KERNEL_BASE	0x00000001		// base kernel code & data
#define REGION_T_DESCRIPTORS	0x00000002		// contains more region descriptors
#define REGION_T_PAGETABLE		0x00000010		// used to map a page table/page directory
#define REGION_T_CORE_HEAP		0x00000100		// used for the core kernel heap
#define REGION_T_PROC_HEAP		0x00000200		// used for a kernel process' heap
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

void dbg_print_region_stats();
