#include <stdbool.h>

#include <debug.h>

#include <syscall.h>

void panic(const char* msg, const char* file, int line) {
	dbg_printf("Panic '%s', at %s:%d\n", msg, file, line);
	exit(-1);
	while(true);
}

void panic_assert(const char* assert, const char* file, int line) {
	dbg_printf("Assert failed '%s', at %s:%d\n", assert, file, line);
	exit(-1);
	while(true);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
