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

void breakpoint_handler(registers_t *regs) {
	dbg_printf("Breakpoint! (int3)\n");
	BOCHS_BREAKPOINT;
}

void test_pf_handler(pagedir_t *pd, region_info_t *i, void* addr) {
	dbg_printf("0x%p", addr);

	uint32_t f = frame_alloc(1);
	if (f == 0) PANIC("Out Of Memory");
	dbg_printf(" -> %i", f);

	int error = pd_map_page(addr, f, 1);
	if (error) PANIC("Could not map frame (OOM)");
}

void* page_alloc_fun_for_kmalloc(size_t bytes) {
	return region_alloc(bytes, REGION_T_CORE_HEAP, test_pf_handler);
}
void page_free_fun_for_kmalloc(void* ptr) {
	region_free(ptr);
}
slab_type_t slab_sizes[] = {
	{ "8B obj", 8, 1 },
	{ "16B obj", 16, 2 },
	{ "32B obj", 32, 2 },
	{ "64B obj", 64, 2 },
	{ "128B obj", 128, 2 },
	{ "256B obj", 256, 4 },
	{ "512B obj", 512, 4 },
	{ "1KB obj", 1024, 8 },
	{ "2KB obj", 2048, 8 },
	{ "4KB obj", 4096, 16 },
	{ 0, 0, 0 }
};


extern char k_end_addr;	// defined in linker script : 0xC0000000 plus kernel stuff
 
void kmain(struct multiboot_info_t *mbd, int32_t mb_magic) {
	dbglog_setup();

	dbg_printf("Hello, kernel World!\n");
	dbg_printf("This is %s, version %s.\n", OS_NAME, OS_VERSION);

	ASSERT(mb_magic == MULTIBOOT_BOOTLOADER_MAGIC);

	gdt_init(); dbg_printf("GDT set up.\n");

	idt_init(); dbg_printf("IDT set up.\n");
	idt_set_ex_handler(EX_BREAKPOINT, breakpoint_handler);
	// asm volatile("int $0x3");	// test breakpoint

	size_t total_ram = ((mbd->mem_upper + mbd->mem_lower) * 1024);
	dbg_printf("Total ram: %d Kb\n", total_ram / 1024);
	// paging_init(totalRam);

	// used for allocation of data structures before malloc is set up
	// a pointer to this pointer is passed to the functions that might have
	// to allocate memory ; they just increment it of the allocated quantity
	void* kernel_data_end = &k_end_addr;

	frame_init_allocator(total_ram, &kernel_data_end);
	dbg_printf("kernel_data_end: 0x%p\n", kernel_data_end);
	dbg_print_frame_stats();

	paging_setup(kernel_data_end);
	dbg_printf("Paging seems to be working!\n");

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

	// allocate a big region and try to write into it
	const size_t n = 1000;
	void* p0 = region_alloc(n * PAGE_SIZE, REGION_T_HW, test_pf_handler);
	for (size_t i = 0; i < n; i++) {
		uint32_t *x = (uint32_t*)(p0 + i * PAGE_SIZE);
		dbg_printf("[%i : ", i);
		x[0] = 12;
		dbg_printf(" : .");
		x[1] = (i * 20422) % 122;
		dbg_printf("]\n", i);
	}
	// unmap memory
	for (size_t i = 0; i < n; i++) {
		uint32_t *x = (uint32_t*)(p0 + i * PAGE_SIZE);
		ASSERT(x[1] == (i * 20422) % 122);

		uint32_t f = pd_get_frame(x);
		ASSERT(f != 0);
		pd_unmap_page(x);

		frame_free(f, 1);
	}
	region_free(s);

	// TEST SLAB ALLOCATOR!!!
	mem_allocator_t *a =
		create_slab_allocator(slab_sizes, page_alloc_fun_for_kmalloc,
										  page_free_fun_for_kmalloc);
	dbg_printf("Created slab allocator at 0x%p\n", a);
	const int m = 100;
	void* ptr[m];
	for (int i = 0; i < m; i++) {
		size_t s = 1 << ((i * 7) % 12 + 1);
		ptr[i] = slab_alloc(a, s);
		dbg_printf("Alloc %i : 0x%p\n", s, ptr[i]);
		dbg_print_region_stats();
	}
	for (int i = 0; i < m; i++) {
		slab_free(a, ptr[m - i - 1]);
	}
	dbg_print_region_stats();
	for (int i = 0; i < m; i++) {
		size_t s = 1 << ((i * 7) % 12 + 1);
		ASSERT(slab_alloc(a, s) == ptr[i]);
	}
	dbg_print_region_stats();
	for (int i = 0; i < m; i++) {
		slab_free(a, ptr[m - i - 1]);
	}
	dbg_print_region_stats();
	dbg_printf("Destroying slab allocator.\n");
	destroy_slab_allocator(a);
	dbg_print_region_stats();


	PANIC("Reached kmain end! Falling off the edge.");
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
