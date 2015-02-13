#include <debug.h>
#include <syscall.h>

#include <string.h>

void dbg_print(const char* str) {

	char buf[256];
	strncpy(buf, str, 256);
	buf[255] = 0;

	asm volatile("int $0x40"::"a"(SC_DBG_PRINT),"b"(buf));
}

void yield() {
	asm volatile("int $0x40"::"a"(SC_YIELD));
}

void exit(int code) {
	asm volatile("int $0x40"::"a"(SC_EXIT),"b"(code));
}
