#include <multiboot.h>
#include <config.h>
#include <dbglog.h>
#include <sys.h>

#include <gdt.h>
#include <idt.h>
#include <frame.h>
#include <paging.h>
#include <region.h>

void breakpoint_handler(registers_t *regs) {
	dbg_printf("Breakpoint! (int3)\n");
	BOCHS_BREAKPOINT;
}
 
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
	// paging_init(totalRam);

	// used for allocation of data structures before malloc is set up
	// a pointer to this pointer is passed to the functions that might have
	// to allocate memory ; they just increment it of the allocated quantity
	void* kernel_data_end = (void*)K_END_ADDR;

	frame_init_allocator(total_ram, &kernel_data_end);
	dbg_printf("kernel_data_end: 0x%p\n", kernel_data_end);
	dbg_print_frame_stats();

	paging_setup(kernel_data_end);
	dbg_printf("Paging seems to be working!\n");

	region_allocator_init(kernel_data_end);
	dbg_print_region_stats();

	size_t p = region_alloc(0x1000, REGION_T_HW, 0);
	dbg_printf("Allocated one-page region: 0x%p\n", p);
	dbg_print_region_stats();
	size_t q = region_alloc(0x1000, REGION_T_HW, 0);
	dbg_printf("Allocated one-page region: 0x%p\n", q);
	dbg_print_region_stats();
	size_t r = region_alloc(0x2000, REGION_T_HW, 0);
	dbg_printf("Allocated two-page region: 0x%p\n", r);
	dbg_print_region_stats();
	size_t s = region_alloc(0x10000, REGION_T_CORE_HEAP, 0);
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

	// TODO:
	// - setup allocator for physical pages (eg: buddy allocator, see OSDev wiki)
	// - setup allocator for virtual memory space
	// - setup paging

	PANIC("Reached kmain end! Falling off the edge.");
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
