#include <multiboot.h>
#include <config.h>
#include <dbglog.h>
#include <sys.h>
#include <malloc.h>

#include <gdt.h>
#include <idt.h>
#include <frame.h>
#include <paging.h>
#include <region.h>
#include <kmalloc.h>

#include <worker.h>
#include <thread.h>

#include <vfs.h>
#include <nullfs.h>
#include <process.h>
#include <elf.h>
#include <sct.h>

#include <slab_alloc.h>
#include <string.h>

#include <dev/pci.h>
#include <dev/pciide.h>

// ===== FOR TESTS =====
#define TEST_PLACEHOLDER_AFTER_IDT
#define TEST_PLACEHOLDER_AFTER_REGION
#define TEST_PLACEHOLDER_AFTER_KMALLOC
#define TEST_PLACEHOLDER_AFTER_TASKING
#define TEST_PLACEHOLDER_AFTER_DEVFS
#ifdef BUILD_KERNEL_TEST
#define BEGIN_TEST(n) dbg_printf("(BEGIN-TEST %s)\n", n);
#define TEST_OK { dbg_printf("(TEST-OK)\n"); asm volatile("cli; hlt"); }
#include <test.c>
#endif
// ===== / FOR TESTS ====

extern const void k_end_addr;	// defined in linker script : 0xC0000000 plus kernel stuff

void breakpoint_handler(registers_t *regs) {
	dbg_printf("Breakpoint! (int3)\n");
	dbg_dump_registers(regs);
	BOCHS_BREAKPOINT;
}

void kernel_init_stage2(void* data);
void kmain(multiboot_info_t *mbd, int32_t mb_magic) {
	// used for allocation of data structures before malloc is set up
	// a pointer to this pointer is passed to the functions that might have
	// to allocate memory ; they just increment it of the allocated quantity
	void* kernel_data_end = (void*)&k_end_addr;

	dbglog_setup();

	dbg_printf("Hello, kernel world!\n");
	dbg_printf("This is %s, version %s.\n", OS_NAME, OS_VERSION);

	ASSERT(mb_magic == MULTIBOOT_BOOTLOADER_MAGIC);

	// Rewrite multiboot header so that we are in higher half
	// Also check that kernel_data_end is after all modules, otherwise
	// we might overwrite something.
	mbd->cmdline += K_HIGHHALF_ADDR;
	size_t cmdline_end = mbd->cmdline + strlen((char*)mbd->cmdline);
	void* cmdline_end_pa = (void*)((cmdline_end & 0xFFFFF000) + 0x1000);
	if (cmdline_end_pa > kernel_data_end) kernel_data_end = cmdline_end_pa;

	mbd->mods_addr += K_HIGHHALF_ADDR;
	multiboot_module_t *mods = (multiboot_module_t*)mbd->mods_addr;
	for (unsigned i = 0; i < mbd->mods_count; i++) {
		mods[i].mod_start += K_HIGHHALF_ADDR;
		mods[i].mod_end += K_HIGHHALF_ADDR;
		mods[i].string += K_HIGHHALF_ADDR;
		void* mod_end_pa = (void*)((mods[i].mod_end & 0xFFFFF000) + 0x1000);
		if (mod_end_pa > kernel_data_end)
			kernel_data_end = mod_end_pa;
	}

	gdt_init(); dbg_printf("GDT set up.\n");

	idt_init(); dbg_printf("IDT set up.\n");
	idt_set_ex_handler(EX_BREAKPOINT, breakpoint_handler);

	TEST_PLACEHOLDER_AFTER_IDT;

	size_t total_ram = ((mbd->mem_upper + mbd->mem_lower) * 1024);
	dbg_printf("Total ram: %d Kb\n", total_ram / 1024);


	frame_init_allocator(total_ram, &kernel_data_end);
	dbg_printf("kernel_data_end: 0x%p\n", kernel_data_end);
	dbg_print_frame_stats();

	paging_setup(kernel_data_end);
	dbg_printf("Paging seems to be working!\n");

	region_allocator_init(kernel_data_end);
	dbg_printf("Region allocator initialized.\n");
	TEST_PLACEHOLDER_AFTER_REGION;

	kmalloc_setup();
	dbg_printf("Kernel malloc setup ok.\n");
	TEST_PLACEHOLDER_AFTER_KMALLOC;

	setup_syscall_table();
	dbg_printf("System calls setup ok.\n");

	// enter multi-threading mode
	// interrupts are enabled at this moment, so all
	// code run from now on should be preemtible (ie thread-safe)
	threading_setup(kernel_init_stage2, mbd);
	PANIC("Should never come here.");
}

void kernel_init_stage2(void* data) {
	dbg_printf("Threading setup ok.\n");

	multiboot_info_t *mbd = (multiboot_info_t*)data;

	dbg_print_region_info();
	dbg_print_frame_stats();

	// Launch some worker threads
	start_workers(2);

	TEST_PLACEHOLDER_AFTER_TASKING;

	// Create iofs
	register_nullfs_driver();
	fs_t *iofs = make_fs("nullfs", 0, "");
	ASSERT(iofs != 0);

	// Scan for devices
	pci_setup();
	pciide_detect(iofs);

	// Add kernel command line to iofs
	{
		dbg_printf("Kernel command line: '%s'\n", (char*)mbd->cmdline);
		size_t len = strlen((char*)mbd->cmdline);
		ASSERT(nullfs_add_ram_file(iofs, "/cmdline", (char*)mbd->cmdline, len, false, FM_READ));
	}

	// Populate iofs with files for kernel modules
	ASSERT(fs_create(iofs, "/mod", FT_DIR));
	multiboot_module_t *mods = (multiboot_module_t*)mbd->mods_addr;
	for (unsigned i = 0; i < mbd->mods_count; i++) {
		char* modname = (char*)mods[i].string;
		char* e = strchr(modname, ' ');
		if (e != 0) (*e) = 0;  // ignore arguments

		char *b = strrchr(modname, '/');
		if (b != 0) modname = b+1; 		// ignore path

		char name[6 + strlen(modname)];
		strcpy(name, "/mod/");
		strcpy(name+5, modname);

		size_t len = mods[i].mod_end - mods[i].mod_start;

		dbg_printf("Adding module to VFS: '%s' (size %d)\n", name, len);

		ASSERT(nullfs_add_ram_file(iofs, name,
					(char*)mods[i].mod_start,
					len, false, FM_READ));
	}

	TEST_PLACEHOLDER_AFTER_DEVFS;

	// Launch INIT
	fs_handle_t *init_bin = fs_open(iofs, "/mod/init.bin", FM_READ);
	if (init_bin == 0) PANIC("No init.bin module provided!");
	if (!is_elf(init_bin)) PANIC("init.bin is not valid ELF32 binary");

	process_t *init_p = new_process(0);
	ASSERT(init_p != 0);

	bool add_iofs_ok = proc_add_fs(init_p, iofs, "io");
	ASSERT(add_iofs_ok);

	proc_entry_t *e = elf_load(init_bin, init_p);
	if (e == 0) PANIC("Could not load ELF file init.bin");

	unref_file(init_bin);

	start_process(init_p, e);

	// We are done here
	dbg_printf("Reached kmain end! I'll just stop here and do nothing.\n");
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
