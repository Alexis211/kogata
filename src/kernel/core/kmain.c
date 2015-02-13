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

#include <thread.h>

#include <vfs.h>
#include <nullfs.h>
#include <process.h>
#include <elf.h>

#include <slab_alloc.h>
#include <hashtbl.h>
#include <string.h>

extern const void k_end_addr;	// defined in linker script : 0xC0000000 plus kernel stuff

void breakpoint_handler(registers_t *regs) {
	dbg_printf("Breakpoint! (int3)\n");
	dbg_dump_registers(regs);
	BOCHS_BREAKPOINT;
}

void test_breakpoint() {
	dbg_printf("(BEGIN-TEST 'breakpoint-test)\n");
	asm volatile("int $0x3");	// test breakpoint
	dbg_printf("(TEST-OK)\n");
}

void test_region_1() {
	dbg_printf("(BEGIN-TEST 'region-test-1)\n");
	void* p = region_alloc(0x1000, "Test region", 0);
	dbg_printf("Allocated one-page region: 0x%p\n", p);
	dbg_print_region_info();
	void* q = region_alloc(0x1000, "Test region", 0);
	dbg_printf("Allocated one-page region: 0x%p\n", q);
	dbg_print_region_info();
	void* r = region_alloc(0x2000, "Test region", 0);
	dbg_printf("Allocated two-page region: 0x%p\n", r);
	dbg_print_region_info();
	void* s = region_alloc(0x10000, "Test region", 0);
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

	dbg_printf("(TEST-OK)\n");
}

void test_region_2() {
	// allocate a big region and try to write into it
	dbg_printf("(BEGIN-TEST 'region-test-2)\n");
	const size_t n = 200;
	void* p0 = region_alloc(n * PAGE_SIZE, "Test big region", default_allocator_pf_handler);
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

	dbg_printf("(TEST-OK)\n");
}

void kmalloc_test(void* kernel_data_end) {
	dbg_printf("(BEGIN-TEST 'kmalloc-test)\n");
	// Test kmalloc !
	dbg_print_region_info();
	const int m = 200;
	uint16_t** ptr = malloc(m * sizeof(uint32_t));
	for (int i = 0; i < m; i++) {
		size_t s = 1 << ((i * 7) % 11 + 2);
		ptr[i] = (uint16_t*)malloc(s);
		ASSERT((void*)ptr[i] >= kernel_data_end && (size_t)ptr[i] < 0xFFC00000);
		*ptr[i] = ((i * 211) % 1024);
	}
	dbg_printf("Fully allocated.\n");
	dbg_print_region_info();
	for (int i = 0; i < m; i++) {
		for (int j = i; j < m; j++) {
			ASSERT(*ptr[j] == (j * 211) % 1024);
		}
		free(ptr[i]);
	}
	free(ptr);
	dbg_printf("Kmalloc test OK.\n");
	dbg_print_region_info();

	dbg_printf("(TEST-OK)\n");
}

void test_hashtbl_1() {
	dbg_printf("(BEGIN-TEST 'test-hashtbl-1)\n");
	// hashtable test
	hashtbl_t *ht = create_hashtbl(str_key_eq_fun, str_hash_fun, 0, 0);
	ASSERT(ht != 0);

	ASSERT(hashtbl_add(ht, "test1", "STRTEST1"));
	ASSERT(hashtbl_add(ht, "test2", "STRTEST2"));
	ASSERT(hashtbl_find(ht, "test1") != 0 && 
			strcmp(hashtbl_find(ht, "test1"), "STRTEST1") == 0);
	ASSERT(hashtbl_find(ht, "test2") != 0 &&
			strcmp(hashtbl_find(ht, "test2"), "STRTEST2") == 0);
	ASSERT(hashtbl_find(ht, "test") == 0);

	ASSERT(hashtbl_add(ht, "test", "Forever alone"));
	ASSERT(hashtbl_find(ht, "test1") != 0 &&
			strcmp(hashtbl_find(ht, "test1"), "STRTEST1") == 0);
	ASSERT(hashtbl_find(ht, "test2") != 0 &&
			strcmp(hashtbl_find(ht, "test2"), "STRTEST2") == 0);
	ASSERT(hashtbl_find(ht, "test") != 0 &&
			strcmp(hashtbl_find(ht, "test"), "Forever alone") == 0);

	hashtbl_remove(ht, "test1");
	ASSERT(hashtbl_find(ht, "test1") == 0);
	ASSERT(hashtbl_find(ht, "test2") != 0 &&
			strcmp(hashtbl_find(ht, "test2"), "STRTEST2") == 0);
	ASSERT(hashtbl_find(ht, "test") != 0 &&
			strcmp(hashtbl_find(ht, "test"), "Forever alone") == 0);

	delete_hashtbl(ht, 0);

	dbg_printf("(TEST-OK)\n");
}

void test_hashtbl_2() {
	dbg_printf("(BEGIN-TEST 'test-hashtbl-2)\n");

	hashtbl_t *ht = create_hashtbl(id_key_eq_fun, id_hash_fun, 0, 0);
	ASSERT(ht != 0);

	ASSERT(hashtbl_add(ht, (void*)12, "TESTSTR12"));
	ASSERT(hashtbl_add(ht, (void*)777, "TESTSTR777"));

	ASSERT(hashtbl_find(ht, (void*)12) != 0 &&
			strcmp(hashtbl_find(ht, (void*)12), "TESTSTR12") == 0);
	ASSERT(hashtbl_find(ht, (void*)777) != 0 &&
			strcmp(hashtbl_find(ht, (void*)777), "TESTSTR777") == 0);
	ASSERT(hashtbl_find(ht, (void*)144) == 0);

	ASSERT(hashtbl_add(ht, (void*)144, "Forever alone"));

	ASSERT(hashtbl_find(ht, (void*)12) != 0 &&
			strcmp(hashtbl_find(ht, (void*)12), "TESTSTR12") == 0);
	ASSERT(hashtbl_find(ht, (void*)144) != 0 &&
			strcmp(hashtbl_find(ht, (void*)144), "Forever alone") == 0);
	ASSERT(hashtbl_find(ht, (void*)777) != 0 &&
			strcmp(hashtbl_find(ht, (void*)777), "TESTSTR777") == 0);

	hashtbl_remove(ht, (void*)12);
	ASSERT(hashtbl_find(ht, (void*)12) == 0);
	ASSERT(hashtbl_find(ht, (void*)144) != 0 &&
			strcmp(hashtbl_find(ht, (void*)144), "Forever alone") == 0);
	ASSERT(hashtbl_find(ht, (void*)777) != 0 &&
			strcmp(hashtbl_find(ht, (void*)777), "TESTSTR777") == 0);

	delete_hashtbl(ht, 0);

	dbg_printf("(TEST-OK)\n");
}

void test_cmdline(multiboot_info_t *mbd, fs_t *devfs) {
	dbg_printf("(BEGIN-TEST 'test-cmdline)\n");

	fs_handle_t *f = fs_open(devfs, "/cmdline", FM_READ);
	ASSERT(f != 0);

	char buf[256];
	size_t l = file_read(f, 0, 255, buf);
	ASSERT(l > 0);
	buf[l] = 0;

	unref_file(f);
	dbg_printf("Command line as in /cmdline file: '%s'.\n", buf);

	ASSERT(strcmp(buf, (char*)mbd->cmdline) == 0);

	dbg_printf("(TEST-OK)\n");
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

	test_breakpoint();

	size_t total_ram = ((mbd->mem_upper + mbd->mem_lower) * 1024);
	dbg_printf("Total ram: %d Kb\n", total_ram / 1024);


	frame_init_allocator(total_ram, &kernel_data_end);
	dbg_printf("kernel_data_end: 0x%p\n", kernel_data_end);
	dbg_print_frame_stats();

	paging_setup(kernel_data_end);
	dbg_printf("Paging seems to be working!\n");

	BOCHS_BREAKPOINT;

	region_allocator_init(kernel_data_end);
	test_region_1();
	test_region_2();

	kmalloc_setup();
	kmalloc_test(kernel_data_end);

	// enter multi-threading mode
	// interrupts are enabled at this moment, so all
	// code run from now on should be preemtible (ie thread-safe)
	threading_setup(kernel_init_stage2, mbd);
	PANIC("Should never come here.");
}

void kernel_init_stage2(void* data) {
	multiboot_info_t *mbd = (multiboot_info_t*)data;

	dbg_print_region_info();
	dbg_print_frame_stats();

	test_hashtbl_1();
	test_hashtbl_2();

	// Create devfs
	register_nullfs_driver();
	fs_t *devfs = make_fs("nullfs", 0, "cd");
	ASSERT(devfs != 0);

	// Add kernel command line to devfs
	{
		dbg_printf("Kernel command line: '%s'\n", (char*)mbd->cmdline);
		size_t len = strlen((char*)mbd->cmdline);
		fs_handle_t* cmdline = fs_open(devfs, "/cmdline", FM_WRITE | FM_CREATE);
		ASSERT(cmdline != 0);
		ASSERT(file_write(cmdline, 0, len, (char*)mbd->cmdline) == len);
		unref_file(cmdline);
	}

	// Populate devfs with files for kernel modules
	ASSERT(fs_create(devfs, "/mod", FT_DIR));
	multiboot_module_t *mods = (multiboot_module_t*)mbd->mods_addr;
	for (unsigned i = 0; i < mbd->mods_count; i++) {
		char* modname = (char*)mods[i].string;
		char* e = strchr(modname, ' ');
		if (e != 0) (*e) = 0;  // ignore arguments

		char *b = strrchr(modname, '/');
		if (b != 0) modname = b+1; 		// ignore path

		char name[6 + strlen(b)];
		strcpy(name, "/mod/");
		strcpy(name+5, modname);

		size_t len = mods[i].mod_end - mods[i].mod_start;

		dbg_printf("Adding module to VFS: '%s' (size %d)\n", name, len);

		/*
		// This would be the "good" way of doing it :
		fs_handle_t* mod_f = fs_open(devfs, name, FM_WRITE | FM_CREATE);
		ASSERT(mod_f != 0);
		ASSERT(file_write(mod_f, 0, len, (char*)mods[i].mod_start) == len);
		unref_file(mod_f);
		*/
		// But since we have a nullfs, we can do it that way to prevent useless data copies :
		ASSERT(nullfs_add_ram_file(devfs, name,
					(char*)mods[i].mod_start,
					len, false, FM_READ | FM_MMAP));
	}

	test_cmdline(mbd, devfs);

	fs_handle_t *init_bin = fs_open(devfs, "/mod/init.bin", FM_READ | FM_MMAP);
	if (init_bin == 0) PANIC("No init.bin module provided!");
	if (!is_elf(init_bin)) PANIC("init.bin is not valid ELF32 binary");

	process_t *init_p = new_process(0);
	ASSERT(init_p != 0);

	bool add_devfs_ok = proc_add_fs(init_p, devfs, "dev");
	ASSERT(add_devfs_ok);

	proc_entry_t *e = elf_load(init_bin, init_p);
	if (e == 0) PANIC("Could not load ELF file init.bin");

	unref_file(init_bin);

	start_process(init_p, e);

	//TODO :
	// - (OK) populate devfs with information regarding kernel command line & modules
	// - create user process with init module provided on command line
	// - give it rights to devfs
	// - launch it
	// - just return, this thread is done

	dbg_printf("Reached kmain end! I'll just stop here and do nothing.\n");
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
