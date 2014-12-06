#include <multiboot.h>
#include <config.h>
#include <dbglog.h>
#include <sys.h>

#include <gdt.h>
#include <idt.h>
#include <frame.h>
#include <paging.h>
#include <region.h>

#include <slab_alloc.h>

extern char k_end_addr;	// defined in linker script : 0xC0000000 plus kernel stuff

void breakpoint_handler(registers_t *regs) {
	dbg_printf("Breakpoint! (int3)\n");
	BOCHS_BREAKPOINT;
}

void* page_alloc_fun_for_kmalloc(size_t bytes) {
	void* addr = region_alloc(bytes, REGION_T_CORE_HEAP, default_allocator_pf_handler);
	dbg_printf("[alloc 0x%p for kmalloc : %p]\n", bytes, addr);
	return addr;
}

void yield() {
	// multitasking not implemented yet
	dbg_printf("Warning : probable deadlock?\n");
}

slab_type_t slab_sizes[] = {
	{ "8B obj", 8, 2 },
	{ "16B obj", 16, 2 },
	{ "32B obj", 32, 2 },
	{ "64B obj", 64, 4 },
	{ "128B obj", 128, 4 },
	{ "256B obj", 256, 4 },
	{ "512B obj", 512, 8 },
	{ "1KB obj", 1024, 8 },
	{ "2KB obj", 2048, 16 },
	{ "4KB obj", 4096, 16 },
	{ 0, 0, 0 }
};

 
void kmain(struct multiboot_info_t *mbd, int32_t mb_magic) {
	dbglog_setup();

	dbg_printf("Hello, kernel World!\n");
	dbg_printf("This is %s, version %s.\n", OS_NAME, OS_VERSION);

	ASSERT(mb_magic == MULTIBOOT_BOOTLOADER_MAGIC);

	gdt_init(); dbg_printf("GDT set up.\n");

	idt_init(); dbg_printf("IDT set up.\n");
	idt_set_ex_handler(EX_BREAKPOINT, breakpoint_handler);
	asm volatile("int $0x3");	// test breakpoint

	size_t total_ram = ((mbd->mem_upper + mbd->mem_lower) * 1024);
	dbg_printf("Total ram: %d Kb\n", total_ram / 1024);

	// used for allocation of data structures before malloc is set up
	// a pointer to this pointer is passed to the functions that might have
	// to allocate memory ; they just increment it of the allocated quantity
	void* kernel_data_end = &k_end_addr;

	frame_init_allocator(total_ram, &kernel_data_end);
	dbg_printf("kernel_data_end: 0x%p\n", kernel_data_end);
	dbg_print_frame_stats();

	paging_setup(kernel_data_end);
	dbg_printf("Paging seems to be working!\n");

	BOCHS_BREAKPOINT;

	region_allocator_init(kernel_data_end);
	dbg_print_region_stats();

	void* p = region_alloc(0x1000, REGION_T_HW, 0);
	dbg_printf("Allocated one-page region: 0x%p\n", p);
	dbg_print_region_stats();
	void* q = region_alloc(0x1000, REGION_T_HW, 0);
	dbg_printf("Allocated one-page region: 0x%p\n", q);
	dbg_print_region_stats();
	void* r = region_alloc(0x2000, REGION_T_HW, 0);
	dbg_printf("Allocated two-page region: 0x%p\n", r);
	dbg_print_region_stats();
	void* s = region_alloc(0x10000, REGION_T_CORE_HEAP, 0);
	dbg_printf("Allocated 16-page region: 0x%p\n", s);
	dbg_print_region_stats();
	region_free(p);
	dbg_printf("Freed region 0x%p\n", p);
	dbg_print_region_stats();
	region_free(q);
	dbg_printf("Freed region 0x%p\n", q);
	dbg_print_region_stats();
	region_free(r);
	dbg_printf("Freed region 0x%p\n", r);
	dbg_print_region_stats();
	region_free(s);
	dbg_printf("Freed region 0x%p\n", s);
	dbg_print_region_stats();
	BOCHS_BREAKPOINT;

	// allocate a big region and try to write into it
	const size_t n = 200;
	void* p0 = region_alloc(n * PAGE_SIZE, REGION_T_HW, default_allocator_pf_handler);
	for (size_t i = 0; i < n; i++) {
		uint32_t *x = (uint32_t*)(p0 + i * PAGE_SIZE);
		dbg_printf("[%i : ", i);
		x[0] = 12;
		dbg_printf(" : .");
		x[1] = (i * 20422) % 122;
		dbg_printf("]\n", i);
	}
	BOCHS_BREAKPOINT;
	// unmap memory
	for (size_t i = 0; i < n; i++) {
		void* p = p0 + i * PAGE_SIZE;
		uint32_t *x = (uint32_t*)p;
		ASSERT(x[1] == (i * 20422) % 122);

		uint32_t f = pd_get_frame(p);
		ASSERT(f != 0);
		pd_unmap_page(p);
		ASSERT(pd_get_frame(p) == 0);

		frame_free(f, 1);
	}
	region_free(s);
	BOCHS_BREAKPOINT;

	// Test slab allocator !
	mem_allocator_t *a =
		create_slab_allocator(slab_sizes, page_alloc_fun_for_kmalloc,
										  region_free_unmap_free);
	dbg_printf("Created slab allocator at 0x%p\n", a);
	dbg_print_region_stats();
	const int m = 200;
	uint16_t** ptr = slab_alloc(a, m * sizeof(uint32_t));
	for (int i = 0; i < m; i++) {
		size_t s = 1 << ((i * 7) % 11 + 2);
		ptr[i] = (uint16_t*)slab_alloc(a, s);
		ASSERT((void*)ptr[i] >= kernel_data_end && (size_t)ptr[i] < 0xFFC00000);
		*ptr[i] = ((i * 211) % 1024);
		dbg_printf("Alloc %i : 0x%p\n", s, ptr[i]);
	}
	dbg_print_region_stats();
	for (int i = 0; i < m; i++) {
		for (int j = i; j < m; j++) {
			ASSERT(*ptr[j] == (j * 211) % 1024);
		}
		slab_free(a, ptr[i]);
	}
	dbg_print_region_stats();
	dbg_printf("Destroying slab allocator...\n");
	destroy_slab_allocator(a);
	dbg_print_region_stats();


	PANIC("Reached kmain end! Falling off the edge.");
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
