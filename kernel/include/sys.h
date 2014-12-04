#pragma once

#include <config.h>

static inline void outb(uint16_t port, uint8_t value) {
	asm volatile("outb %1, %0" : : "dN"(port), "a"(value));
}

static inline void outw(uint16_t port, uint16_t value) {
	asm volatile("outw %1, %0" : : "dN"(port), "a"(value));
}

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
	return ret;
}

static inline uint16_t inw(uint16_t port) {
	uint16_t ret;
	asm volatile("inw %1, %0" : "=a"(ret) : "dN"(port));
	return ret;
}

static inline void invlpg(size_t addr) {
	asm volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

void panic(const char* message, const char* file, int line);
void panic_assert(const char* assertion, const char* file, int line);
#define PANIC(s) panic(s, __FILE__, __LINE__);
#define ASSERT(s) { if (!(s)) panic_assert(#s, __FILE__, __LINE__); }

#define BOCHS_BREAKPOINT asm volatile("xchg %bx, %bx")


// Utility functions for memory alignment

#define PAGE_SIZE		0x1000
#define PAGE_MASK		0xFFFFF000
#define PAGE_ALIGN_DOWN(x)	(((size_t)x) & PAGE_MASK) 
#define PAGE_ALIGN_UP(x)  	((((size_t)x)&(~PAGE_MASK)) == 0 ? ((size_t)x) : (((size_t)x) & PAGE_MASK) + PAGE_SIZE)
#define PAGE_ID(x)			(((size_t)x) / PAGE_SIZE)
#define PAGE_SHIFT		12
#define PT_SHIFT		10
// PAGE_SHIFT + PT_SHIFT + PT_SHIFT = 32
#define N_PAGES_IN_PT		1024
#define PD_MIRROR_ADDR	0xFFC00000		// last 4MB used for PD mirroring
#define LAST_KERNEL_ADDR	PD_MIRROR_ADDR
#define FIRST_KERNEL_PT	(K_HIGHHALF_ADDR >> (PAGE_SHIFT+PT_SHIFT))	// must be 768

#define MASK4			0xFFFFFFFC
#define ALIGN4_UP(x)	((((size_t)x)&(~MASK4)) == 0 ? ((size_t)x) : (((size_t)x) & MASK4) + 4)
#define ALIGN4_DOWN(x)	(((size_t)x)&MASK4)


/* vim: set ts=4 sw=4 tw=0 noet :*/
