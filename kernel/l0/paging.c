#include <paging.h>
#include <frame.h>

typedef union page {
	struct {
		uint32_t present		: 1;
		uint32_t rw				: 1;
		uint32_t user			: 1;
		uint32_t write_through  : 1;
		uint32_t disable_cache	: 1;
		uint32_t accessed		: 1;
		uint32_t dirty			: 1;	// only PTE
		uint32_t size_4m		: 1;	// only PDE
		uint32_t global			: 1;	// only PTE
		uint32_t rsvd			: 3;
		uint32_t frame			: 20;
	};
	uint32_t as_int32;
} page_t;

typedef struct page_table {
	page_t page[1024];
} pagetable_t;

struct page_directory {
	pagetable_t *pt[1024];	// virtual addresses of each page table
	pagetable_t *dir;		// virtual address of page directory
	size_t phys_addr;		// physical address of page directory
};


// access kernel page directory page defined in loader.s
// (this is a correct higher-half address)
extern pagetable_t kernel_pagedir;

static pagetable_t __attribute__((aligned(PAGE_SIZE))) kernel_pt768;
static pagedir_t kernel_pd;

static pagedir_t *current_pd;

void paging_setup(void* kernel_data_end) {
	size_t n_kernel_pages =
		PAGE_ALIGN_UP((size_t)kernel_data_end - K_HIGHHALF_ADDR)/PAGE_SIZE;

	ASSERT(n_kernel_pages <= 1024);

	// setup kernel_pd structure
	kernel_pd.dir = &kernel_pagedir;
	kernel_pd.phys_addr = (size_t)kernel_pd.dir - K_HIGHHALF_ADDR;
	for (size_t i = 0; i < 1024; i++) kernel_pd.pt[i] = 0;

	// setup kernel_pt768
	for (size_t i = 0; i < n_kernel_pages; i++) {
		kernel_pt768.page[i].as_int32 = 0;	// clear any junk
		kernel_pt768.page[i].present = 1;
		kernel_pt768.page[i].user = 0;
		kernel_pt768.page[i].rw = 1;
		kernel_pt768.page[i].frame = i;
	}
	for (size_t i = n_kernel_pages; i < 1024; i++){
		kernel_pt768.page[i].as_int32 = 0;
	}

	// replace 4M mapping by kernel_pt768
	kernel_pd.pt[768] = &kernel_pt768;
	kernel_pd.dir->page[768].as_int32 =
		(((size_t)&kernel_pt768 - K_HIGHHALF_ADDR) & PAGE_MASK) | 0x07;

	current_pd = &kernel_pd;

	// paging already enabled in loader, nothing to do.

	// disable 4M pages (remove PSE bit in CR4)
	uint32_t cr4;
	asm volatile("movl %%cr4, %0": "=r"(cr4));
	cr4 &= ~0x00000010;
	asm volatile("movl %0, %%cr4":: "r"(cr4));

	// TODO : setup page fault handler
}

pagedir_t *get_current_pagedir() {
	return current_pd;
}

pagedir_t *get_kernel_pagedir() {
	return &kernel_pd;
}

void switch_pagedir(pagedir_t *pd) {
    asm volatile("movl %0, %%cr3":: "r"(pd->phys_addr));
}

// ============================== //
// Mapping and unmapping of pages //
// ============================== //

uint32_t pd_get_frame(pagedir_t *pd, size_t vaddr) {
	uint32_t page = vaddr / PAGE_SIZE;
	uint32_t pt = page / PAGE_SIZE;
	uint32_t pt_page = page % PAGE_SIZE;

	if (pd == 0) return 0;
	if (pd->pt[pt] == 0) return 0;
	if (!pd->pt[pt]->page[pt_page].present) return 0;
	return pd->pt[pt]->page[pt_page].frame;
}

int pd_map_page(pagedir_t *pd, size_t vaddr, uint32_t frame_id, uint32_t rw) {
	return 1; 	// TODO
} 

void pd_unmap_page(pagedir_t *pd, size_t vaddr) {
	uint32_t page = vaddr / PAGE_SIZE;
	uint32_t pt = page / PAGE_SIZE;
	uint32_t pt_page = page % PAGE_SIZE;

	if (pd == 0) return;
	if (pd->pt[pt] == 0) return;
	if (!pd->pt[pt]->page[pt_page].present) return;
	pd->pt[pt]->page[pt_page].as_int32 = 0;

	// TODO (?) : if pagetable is completely empty, free it
}

// Creation and deletion of page directories

pagedir_t *create_pagedir() {
	return 0;	// TODO
}
void delete_pagedir(pagedir_t *pd) {
	return;		// TODO
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
