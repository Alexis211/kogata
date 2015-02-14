#include <sys.h>
#include <dbglog.h>


// Kernel panic and kernel assert failure

static void panic_do(const char* type, const char *msg, const char* file, int line) {
	asm volatile("cli;");
	dbg_printf("/\n| %s:\t%s\n", type, msg);
	dbg_printf("| File: \t%s:%i\n", file, line);
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
}

void panic_assert(const char* assertion, const char* file, int line) {
	panic_do("ASSERT FAILED", assertion, file, line);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
