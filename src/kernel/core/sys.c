#include <sys.h>
#include <dbglog.h>
#include <thread.h>
#include <string.h>

#include <elf.h>

#include <btree.h>


// Kernel panic and kernel assert failure

static void panic_do(const char* type, const char *msg, const char* file, int line) {
	asm volatile("cli;");
	dbg_printf("/\n| %s:\t%s\n", type, msg);
	dbg_printf("| File: \t%s:%i\n", file, line);

	dbg_printf("- trace\n");
	dbg_printf("| current thread: 0x%p\n", current_thread);
	uint32_t *ebp;
	asm volatile("mov %%ebp, %0":"=r"(ebp));
	kernel_stacktrace(ebp[0], ebp[1]);

	dbg_print_region_info();

	dbg_printf("| System halted -_-'\n");
	dbg_printf("\\---------------------------------------------------------/");
	BOCHS_BREAKPOINT;
#ifdef BUILD_KERNEL_TEST
	dbg_printf("\n(TEST-FAIL)\n");
#endif
	asm volatile("hlt");
}

void sys_panic(const char* message, const char* file, int line) {
	panic_do("PANIC", message, file, line);
	while(true);
}

void sys_panic_assert(const char* assertion, const char* file, int line) {
	panic_do("ASSERT FAILED", assertion, file, line);
	while(true);
}

//  ---- kernel symbol map

btree_t *kernel_symbol_map = 0;

void load_kernel_symbol_table(elf_shdr_t *sym, elf_shdr_t *str) {
	kernel_symbol_map = create_btree(id_key_cmp_fun, 0);
	ASSERT (kernel_symbol_map != 0);

	dbg_printf("Loading kernel symbol table...\n");

	
	ASSERT(sym->sh_entsize == sizeof(elf_sym_t));
	unsigned nsym = sym->sh_size / sym->sh_entsize;

	elf_sym_t *st = (elf_sym_t*)sym->sh_addr;
	const char* strtab = (const char*)(str->sh_addr);
	for (unsigned j = 0; j < nsym; j++) {
		btree_add(kernel_symbol_map, (void*)st[j].st_value, (void*)(strtab + st[j].st_name));
	}
}

void kernel_stacktrace(uint32_t ebp, uint32_t eip) {
	int i = 0;
	while (ebp >= K_HIGHHALF_ADDR) {
		char* sym = 0;
		void* fn_ptr = 0;
		if (kernel_symbol_map != 0) {
			sym = btree_lower(kernel_symbol_map, (void*)eip, &fn_ptr);
		}

		dbg_printf("| 0x%p	EIP: 0x%p  %s  +%d\n", ebp, eip, sym, ((void*)eip - fn_ptr));

		uint32_t *d = (uint32_t*)ebp;
		ebp = d[0];
		eip = d[1];
		if (i++ == 20) {
			dbg_printf("| ...");
			break;
		}
		if (eip == 0) break;
	}
}


/* vim: set ts=4 sw=4 tw=0 noet :*/
