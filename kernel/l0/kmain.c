#include <multiboot.h>
#include <config.h>
#include <dbglog.h>
#include <sys.h>

#include <gdt.h>
#include <idt.h>
#include <frame.h>
#include <paging.h>
#include <region.h>
#include <kmalloc.h>

#include <task.h>

#include <slab_alloc.h>

extern char k_end_addr;	// defined in linker script : 0xC0000000 plus kernel stuff

void breakpoint_handler(registers_t *regs) {
	dbg_printf("Breakpoint! (int3)\n");
	BOCHS_BREAKPOINT;
}

void region_test1() {
	void* p = region_alloc(0x1000, REGION_T_HW, 0);
	dbg_printf("Allocated one-page region: 0x%p\n", p);
	dbg_print_region_info();
	void* q = region_alloc(0x1000, REGION_T_HW, 0);
	dbg_printf("Allocated one-page region: 0x%p\n", q);
	dbg_print_region_info();
	void* r = region_alloc(0x2000, REGION_T_HW, 0);
	dbg_printf("Allocated two-page region: 0x%p\n", r);
	dbg_print_region_info();
	void* s = region_alloc(0x10000, REGION_T_CORE_HEAP, 0);
	dbg_printf("Allocated 16-page region: 0x%p\n", s);
	dbg_print_region_info();
	region_free(p);
	dbg_printf("Freed region 0x%p\n", p);
	dbg_print_region_info();
	region_free(q);
	dbg_printf("Freed region 0x%p\n", q);
	dbg_print_region_info();
	region_free(r);
	dbg_printf("Freed region 0x%p\n", r);
	dbg_print_region_info();
	region_free(s);
	dbg_printf("Freed region 0x%p\n", s);
	dbg_print_region_info();
}

void region_test2() {
	// allocate a big region and try to write into it
	dbg_printf("Begin region test 2...");
	const size_t n = 200;
	void* p0 = region_alloc(n * PAGE_SIZE, REGION_T_HW, default_allocator_pf_handler);
	for (size_t i = 0; i < n; i++) {
		uint32_t *x = (uint32_t*)(p0 + i * PAGE_SIZE);
		x[0] = 12;
		x[1] = (i * 20422) % 122;
	}
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
	region_free(p0);
	dbg_printf("OK\n");
}

void kmalloc_test(void* kernel_data_end) {
	// Test kmalloc !
	dbg_print_region_info();
	dbg_printf("Begin kmalloc test...\n");
	const int m = 200;
	uint16_t** ptr = kmalloc(m * sizeof(uint32_t));
	for (int i = 0; i < m; i++) {
		size_t s = 1 << ((i * 7) % 11 + 2);
		ptr[i] = (uint16_t*)kmalloc(s);
		ASSERT((void*)ptr[i] >= kernel_data_end && (size_t)ptr[i] < 0xFFC00000);
		*ptr[i] = ((i * 211) % 1024);
	}
	dbg_printf("Fully allocated.\n");
	dbg_print_region_info();
	for (int i = 0; i < m; i++) {
		for (int j = i; j < m; j++) {
			ASSERT(*ptr[j] == (j * 211) % 1024);
		}
		kfree(ptr[i]);
	}
	kfree(ptr);
	dbg_printf("Kmalloc test OK.\n");
	dbg_print_region_info();
}

void test_task(void* a) {
	int i = 0;
	while(1) {
		dbg_printf("b");
		for (int x = 0; x < 100000; x++) asm volatile("xor %%ebx, %%ebx":::"%ebx");
		if (++i == 8) {
			yield();
			i = 0;
		}
	}
}
void kernel_init_stage2(void* data) {
	task_t *tb = new_task(test_task);
	resume_task_with_result(tb, 0, false);

	dbg_print_region_info();
	dbg_print_frame_stats();

	while(1) {
		dbg_printf("a");
		for (int x = 0; x < 100000; x++) asm volatile("xor %%ebx, %%ebx":::"%ebx");
	}
	PANIC("Reached kmain end! Falling off the edge.");
}
 
void kmain(struct multiboot_info_t *mbd, int32_t mb_magic) {
	dbglog_setup();

	dbg_printf("Hello, kernel world!\n");
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
	region_test1();
	region_test2();

	kmalloc_setup();
	kmalloc_test(kernel_data_end);

	// enter multi-tasking mode
	// interrupts are enabled at this moment, so all
	// code run from now on should be preemtible (ie thread-safe)
	tasking_setup(kernel_init_stage2, 0);
	PANIC("Should never come here.");
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
