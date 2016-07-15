#include <stdbool.h>

#include <kogata/debug.h>
#include <kogata/syscall.h>

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

/* vim: set ts=4 sw=4 tw=0 noet :*/
