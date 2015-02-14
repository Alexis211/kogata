#include <debug.h>
#include <syscall.h>

#include <string.h>

void dbg_print(const char* str) {
	asm volatile("int $0x40"::"a"(SC_DBG_PRINT),"b"(str),"c"(strlen(str)));
}

void yield() {
	asm volatile("int $0x40"::"a"(SC_YIELD));
}

void exit(int code) {
	asm volatile("int $0x40"::"a"(SC_EXIT),"b"(code));
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
