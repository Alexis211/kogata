#include <sys.h>
#include <dbglog.h>


// C wrappers for inb/outb/inw/outw

void outb(uint16_t port, uint8_t value) {
	asm volatile("outb %1, %0" : : "dN"(port), "a"(value));
}

void outw(uint16_t port, uint16_t value) {
	asm volatile("outw %1, %0" : : "dN"(port), "a"(value));
}

uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
	return ret;
}

uint16_t inw(uint16_t port) {
	uint16_t ret;
	asm volatile("inw %1, %0" : "=a"(ret) : "dN"(port));
	return ret;
}

// Kernel panic and kernel assert failure

static void panic_do(const char* type, const char *msg, const char* file, int line) {
	asm volatile("cli;");
	dbg_printf("/\n| %s:\t%s\n", type, msg);
	dbg_printf("| File: \t%s:%i\n", file, line);
	dbg_printf("| System halted -_-'\n");
	dbg_printf("\\---------------------------------------------------------/");
	asm volatile("hlt");
}

void panic(const char* message, const char* file, int line) {
	panic_do("PANIC", message, file, line);
}

void panic_assert(const char* assertion, const char* file, int line) {
	panic_do("ASSERT FAILED", assertion, file, line);
}
