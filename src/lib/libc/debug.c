#include <stdbool.h>
#include <stdlib.h>

#include <kogata/syscall.h>
#include <kogata/debug.h>
#include <kogata/printf.h>

void sys_panic(const char* msg, const char* file, int line) {
	dbg_printf("PANIC in user process\n  %s\n  at %s:%d\n", msg, file, line);
	exit(-1);
	while(true);
}

void sys_panic_assert(const char* assert, const char* file, int line) {
	dbg_printf("ASSERT FAILED in user process\n  %s\n   at %s:%d\n", assert, file, line);
	exit(-1);
	while(true);
}

void dbg_print(const char* s) {
	sc_dbg_print(s);
}

void dbg_printf(const char* fmt, ...) {
	va_list ap;
	char buffer[256];

	va_start(ap, fmt);
	vsnprintf(buffer, 256, fmt, ap);
	va_end(ap);

	dbg_print(buffer);
}

void yield() {
	sc_yield();
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
