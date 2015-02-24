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
#include <fs/iso9660.h>

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

btree_t *parse_cmdline(const char* x);
fs_t *setup_iofs(multiboot_info_t *mbd);
fs_t *setup_rootfs(btree_t *cmdline, fs_t *iofs);
void launch_init(btree_t *cmdline, fs_t *iofs, fs_t *rootfs);

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
	fs_t *iofs = setup_iofs(mbd);

	TEST_PLACEHOLDER_AFTER_DEVFS;

	// Scan for devices
	pci_setup();
	pciide_detect(iofs);

	// Register FS drivers
	register_iso9660_driver();

	// Parse command line
	btree_t *cmdline = parse_cmdline((const char*)mbd->cmdline);

	fs_t *rootfs = 0;
	if (btree_find(cmdline, "root") != 0) rootfs = setup_rootfs(cmdline, iofs);

	launch_init(cmdline, iofs, rootfs);

	// We are done here
	dbg_printf("Reached kmain end! I'll just stop here and do nothing.\n");
}

btree_t *parse_cmdline(const char* x) {
	btree_t *ret = create_btree(str_key_cmp_fun, free_key_val);
	ASSERT(ret != 0);

	x = strchr(x, ' ');
	if (x == 0) return ret;

	while ((*x) != 0) {
		while (*x == ' ') x++;

		char* eq = strchr(x, '=');
		char* sep = strchr(x, ' ');
		if (sep == 0) {
			if (eq == 0) {
				btree_add(ret, strdup(x), strdup(x));
			} else {
				btree_add(ret, strndup(x, eq - x), strdup(eq + 1));
			}
			break;
		} else {
			if (eq == 0 || eq > sep) {
				btree_add(ret, strndup(x, sep - x), strndup(x, sep - x));
			} else {
				btree_add(ret, strndup(x, eq - x), strndup(eq + 1, sep - eq - 1));
			}
			x = sep + 1;
		}
	}

	void iter(void* a, void* b) {
		dbg_printf("  '%s'  ->  '%s'\n", a, b);
	}
	btree_iter(ret, iter);
	
	return ret;
}

fs_t *setup_iofs(multiboot_info_t *mbd) {
	fs_t *iofs = make_fs("nullfs", 0, "");
	ASSERT(iofs != 0);

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

	return iofs;
}

fs_t *setup_rootfs(btree_t *cmdline, fs_t *iofs) {
	char* root = (char*)btree_find(cmdline, "root");
	char* root_opts = (char*)btree_find(cmdline, "root_opts");
	if (root == 0) PANIC("No root device specified on kernel command line.");

	char* sep = strchr(root, ':');
	if (sep != 0) {
		ASSERT(root[0] == 'i' && root[1] == 'o' && root[2] == ':');
		root = root + 3;		// ignore prefix, we are always on iofs
	}

	dbg_printf("Trying to open root device %s... ", root);
	fs_handle_t *root_dev = fs_open(iofs, root, FM_READ | FM_WRITE | FM_IOCTL);
	if (root_dev == 0) {
		dbg_printf("read-only... ");
		root_dev = fs_open(iofs, root, FM_READ | FM_WRITE | FM_IOCTL);
	}
	dbg_printf("\n");
	if (root_dev == 0) PANIC("Could not open root device.");

	fs_t *rootfs = make_fs(0, root_dev, (root_opts ? root_opts : ""));
	if (rootfs == 0) PANIC("Could not mount root device.");

	return rootfs;
}

void launch_init(btree_t *cmdline, fs_t *iofs, fs_t *rootfs)  {
	fs_t *init_fs = rootfs;
	char* init_file = btree_find(cmdline, "init");
	if (init_file == 0) PANIC("No init specified on kernel command line.");

	dbg_printf("Launching init %s...\n", init_file);

	char* sep = strchr(init_file, ':');
	if (sep != 0) {
		if (strncmp(init_file, "io:", 3) == 0) {
			init_fs = iofs;
			init_file = init_file + 3;
		} else if (strncmp(init_file, "root:", 5) == 0) {
			init_fs = rootfs;
			init_file = init_file + 5;
		}
	}
	if (init_fs == 0) PANIC("Invalid file system specification for init file.");

	// Launch INIT
	fs_handle_t *init_bin = fs_open(init_fs, init_file, FM_READ);
	if (init_bin == 0) PANIC("Could not open init file.");
	if (!is_elf(init_bin)) PANIC("init.bin is not valid ELF32 binary");

	process_t *init_p = new_process(0);
	ASSERT(init_p != 0);

	bool add_iofs_ok = proc_add_fs(init_p, iofs, "io");
	ASSERT(add_iofs_ok);
	bool add_rootfs_ok = proc_add_fs(init_p, rootfs, "root");
	ASSERT(add_rootfs_ok);

	proc_entry_t *e = elf_load(init_bin, init_p);
	if (e == 0) PANIC("Could not load init ELF file");

	unref_file(init_bin);

	start_process(init_p, e);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
