#include <paging.h>
#include <frame.h>
#include <idt.h>
#include <dbglog.h>
#include <region.h>

#define PAGE_OF_ADDR(x)		(((size_t)x >> PAGE_SHIFT) % N_PAGES_IN_PT)
#define PT_OF_ADDR(x)		((size_t)x >> (PAGE_SHIFT + PT_SHIFT))

#define PTE_PRESENT			(1<<0)
#define PTE_RW				(1<<1)
#define PTE_USER			(1<<2)
#define PTE_WRITE_THROUGH	(1<<3)
#define PTE_DISABLE_CACHE	(1<<4)
#define PTE_ACCESSED		(1<<5)
#define PTE_DIRTY			(1<<6)		// only PTE
#define PTE_SIZE_4M			(1<<7)		// only PDE
#define PTE_GLOBAL			(1<<8)		// only PTE
#define PTE_FRAME_SHIFT		12

typedef struct page_table {
	uint32_t page[1024];
} pagetable_t;

struct page_directory {
	uint32_t phys_addr;		// physical address of page directory
	// to modify a page directory, we first map it
	// then we can use mirroring to edit it
	// (the last 4M of the address space are mapped to the PD itself)

	// more info to be stored here, potentially
};


// access kernel page directory page defined in loader.s
// (this is a correct higher-half address)
extern pagetable_t kernel_pd;

// pre-allocate a page table so that we can map the first 4M of kernel memory
static pagetable_t __attribute__((aligned(PAGE_SIZE))) kernel_pt0;

static pagedir_t kernel_pd_d;
static pagedir_t *current_pd_d;

#define current_pt ((pagetable_t*)PD_MIRROR_ADDR)
#define current_pd ((pagetable_t*)(PD_MIRROR_ADDR + (N_PAGES_IN_PT-1)*PAGE_SIZE))

void page_fault_handler(registers_t *regs) {
	void* vaddr;
	asm volatile("movl %%cr2, %0":"=r"(vaddr));

	if ((size_t)vaddr >= K_HIGHHALF_ADDR) {
		uint32_t pt = PT_OF_ADDR(vaddr);

		if (current_pd != &kernel_pd && current_pd->page[pt] != kernel_pd.page[pt]) {
			current_pd->page[pt] = kernel_pd.page[pt];
			invlpg(&current_pt[pt]);
			return;
		}

		if ((size_t)vaddr >= PD_MIRROR_ADDR) {
			dbg_printf("Fault on access to mirrorred PD at 0x%p\n", vaddr);

			uint32_t x = (size_t)vaddr - PD_MIRROR_ADDR;
			uint32_t page = (x % PAGE_SIZE) / 4;
			uint32_t pt = x / PAGE_SIZE;
			dbg_printf("For pt 0x%p, page 0x%p -> addr 0x%p\n", pt, page, ((pt * 1024) + page) * PAGE_SIZE);

			for (int i = 0; i < N_PAGES_IN_PT; i++) {
				//dbg_printf("%i. 0x%p\n", i, kernel_pd.page[i]);
			}

			dbg_dump_registers(regs);
			dbg_print_region_stats();
			PANIC("Unhandled kernel space page fault");
		}

		region_info_t *i = find_region(vaddr);
		if (i == 0) {
			dbg_printf("Kernel pagefault in non-existing region at 0x%p\n", vaddr);
			dbg_dump_registers(regs);
			PANIC("Unhandled kernel space page fault");
		}
		if (i->pf == 0) {
			dbg_printf("Kernel pagefault in region with no handler at 0x%p\n", vaddr);
			dbg_dump_registers(regs);
			PANIC("Unhandled kernel space page fault");
		}
		i->pf(current_pd_d, i, vaddr);
	} else {
		dbg_printf("Userspace page fault at 0x%p\n", vaddr);
		PANIC("Unhandled userspace page fault");
		// not handled yet
		// TODO
	}
}

void paging_setup(void* kernel_data_end) {
	size_t n_kernel_pages =
		PAGE_ALIGN_UP((size_t)kernel_data_end - K_HIGHHALF_ADDR)/PAGE_SIZE;

	ASSERT(n_kernel_pages <= 1024);	// we use less than 4M for kernel

	// setup kernel_pd_d structure
	kernel_pd_d.phys_addr = (size_t)&kernel_pd - K_HIGHHALF_ADDR;

	// setup kernel_pt0
	ASSERT(PAGE_OF_ADDR(K_HIGHHALF_ADDR) == 0);	// kernel is 4M-aligned
	ASSERT(FIRST_KERNEL_PT == 768);
	for (size_t i = 0; i < n_kernel_pages; i++) {
		kernel_pt0.page[i] = (i << PTE_FRAME_SHIFT) | PTE_PRESENT | PTE_RW | PTE_GLOBAL;
	}
	for (size_t i = n_kernel_pages; i < 1024; i++){
		kernel_pt0.page[i] = 0;
	}

	// replace 4M mapping by kernel_pt0
	kernel_pd.page[FIRST_KERNEL_PT] =
		(((size_t)&kernel_pt0 - K_HIGHHALF_ADDR) & PAGE_MASK) | PTE_PRESENT | PTE_RW;
	// set up mirroring
	kernel_pd.page[N_PAGES_IN_PT-1] =
		(((size_t)&kernel_pd - K_HIGHHALF_ADDR) & PAGE_MASK) | PTE_PRESENT | PTE_RW;

	invlpg((void*)K_HIGHHALF_ADDR);

	// paging already enabled in loader, nothing to do.
	switch_pagedir(&kernel_pd_d);

	// disable 4M pages (remove PSE bit in CR4)
	uint32_t cr4;
	asm volatile("movl %%cr4, %0": "=r"(cr4));
	cr4 &= ~0x00000010;
	asm volatile("movl %0, %%cr4":: "r"(cr4));

	idt_set_ex_handler(EX_PAGE_FAULT, page_fault_handler);
}

pagedir_t *get_current_pagedir() {
	return current_pd_d;
}

pagedir_t *get_kernel_pagedir() {
	return &kernel_pd_d;
}

void switch_pagedir(pagedir_t *pd) {
	asm volatile("movl %0, %%cr3":: "r"(pd->phys_addr));
	invlpg(current_pd);
	current_pd_d = pd;
}

// ============================== //
// Mapping and unmapping of pages //
// ============================== //

uint32_t pd_get_frame(void* vaddr) {
	uint32_t pt = PT_OF_ADDR(vaddr);
	uint32_t page = PAGE_OF_ADDR(vaddr);

	pagetable_t *pd = ((size_t)vaddr >= K_HIGHHALF_ADDR ? &kernel_pd : current_pd);

	if (!pd->page[pt] & PTE_PRESENT) return 0;
	if (!current_pt[pt].page[page] & PTE_PRESENT) return 0;
	return current_pt[pt].page[page] >> PTE_FRAME_SHIFT;
}

int pd_map_page(void* vaddr, uint32_t frame_id, bool rw) {
	uint32_t pt = PT_OF_ADDR(vaddr);
	uint32_t page = PAGE_OF_ADDR(vaddr);

	ASSERT((size_t)vaddr < PD_MIRROR_ADDR);
	
	pagetable_t *pd = ((size_t)vaddr >= K_HIGHHALF_ADDR ? &kernel_pd : current_pd);

	if (!pd->page[pt] & PTE_PRESENT) {
		uint32_t new_pt_frame = frame_alloc(1);
		if (new_pt_frame == 0) return 1;	// OOM

		current_pd->page[pt] = pd->page[pt] =
			(new_pt_frame << PTE_FRAME_SHIFT) | PTE_PRESENT | PTE_RW;
		invlpg(&current_pt[pt]);
	}
	dbg_printf("[%p,%i,%i,", vaddr, pt, page);

	current_pt[pt].page[page] =
		(frame_id << PTE_FRAME_SHIFT)
			| PTE_PRESENT
			| ((size_t)vaddr < K_HIGHHALF_ADDR ? PTE_USER : PTE_GLOBAL)
			| (rw ? PTE_RW : 0);
	invlpg(vaddr);

	dbg_printf("]");

	return 0;
} 

void pd_unmap_page(void* vaddr) {
	uint32_t pt = PT_OF_ADDR(vaddr);
	uint32_t page = PAGE_OF_ADDR(vaddr);

	pagetable_t *pd = ((size_t)vaddr >= K_HIGHHALF_ADDR ? &kernel_pd : current_pd);

	if (!pd->page[pt] & PTE_PRESENT) return;
	if (!current_pt[pt].page[page] & PTE_PRESENT) return;

	current_pt[pt].page[page] = 0;
	invlpg(vaddr);

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