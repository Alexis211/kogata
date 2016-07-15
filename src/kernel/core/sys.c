#include <sys.h>
#include <dbglog.h>
#include <thread.h>
#include <string.h>

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

void panic(const char* message, const char* file, int line) {
	panic_do("PANIC", message, file, line);
	while(true);
}

void panic_assert(const char* assertion, const char* file, int line) {
	panic_do("ASSERT FAILED", assertion, file, line);
	while(true);
}

//  ---- kernel symbol map

btree_t *kernel_symbol_map = 0;

void load_kernel_symbol_map(char* text, size_t len) {
	kernel_symbol_map = create_btree(id_key_cmp_fun, 0);
	ASSERT (kernel_symbol_map != 0);

	dbg_printf("Loading kernel symbol map...\n");

	char* it = text;
	while (it < text + len) {
		char* eol = it;
		while (eol < text + len && *eol != 0 && *eol != '\n') eol++;
		if (eol >= text + len) break;
		*eol = 0;

		if (it[16] == '0' && it[17] == 'x' && it[34] == ' ' && it[49] == ' ') {
			uint32_t addr = 0;
			for (unsigned i = 18; i < 34; i++) {
				addr *= 16;
				if (it[i] >= '0' && it[i] <= '9') addr += it[i] - '0';
				if (it[i] >= 'a' && it[i] <= 'f') addr += it[i] - 'a' + 10;
			}
			btree_add(kernel_symbol_map, (void*)addr, it + 50);
		}

		it = eol + 1;
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
