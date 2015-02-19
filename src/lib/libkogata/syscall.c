#include <debug.h>
#include <syscall.h>

#include <string.h>
#include <printf.h>

static uint32_t call(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t ss, uint32_t dd) {
	uint32_t ret;
	asm volatile("int $0x40"
		:"=a"(ret)
		:"a"(a),"b"(b),"c"(c),"d"(d),"S"(ss),"D"(dd));
	return ret;
}

void dbg_print(const char* str) {
	call(SC_DBG_PRINT, (uint32_t)str, strlen(str), 0, 0, 0);
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
	call(SC_YIELD, 0, 0, 0, 0, 0);
}

void exit(int code) {
	call(SC_EXIT, code, 0, 0, 0, 0);
}

bool mmap(void* addr, size_t size, int mode) {
	return call(SC_MMAP, (uint32_t)addr, size, mode, 0, 0);
}
bool mchmap(void* addr, int mode) {
	return call(SC_MCHMAP, (uint32_t)addr, mode, 0, 0, 0);
}
bool munmap(void* addr) {
	return call(SC_MUNMAP, (uint32_t)addr, 0, 0, 0, 0);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
