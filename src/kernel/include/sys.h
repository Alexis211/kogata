#pragma once

#include <debug.h> 		// common header
#include <config.h>

typedef struct elf_shdr elf_shdr_t;

static inline void outb(uint16_t port, uint8_t value) {
	asm volatile("outb %1, %0" : : "dN"(port), "a"(value));
}

static inline void outw(uint16_t port, uint16_t value) {
	asm volatile("outw %1, %0" : : "dN"(port), "a"(value));
}

static inline void outl(uint16_t port, uint32_t value) {
	asm volatile("outl %1, %0" : : "dN"(port), "a"(value));
}

static inline void outsb(uint16_t port, const void *addr, size_t cnt) {
	// write cnt bytes to port
	asm volatile ("rep outsb" : "+S" (addr), "+c" (cnt) : "d" (port));
}

static inline void outsw(uint16_t port, const void *addr, size_t cnt) {
	// write cnt words to port
	asm volatile ("rep outsw" : "+S" (addr), "+c" (cnt) : "d" (port));
}

static inline void outsl (uint16_t port, const void *addr, size_t cnt) {
	// write cnt longwords to port
	asm volatile ("rep outsl" : "+S" (addr), "+c" (cnt) : "d" (port));
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

static inline uint32_t inl(uint16_t port) {
	uint32_t ret;
	asm volatile("inl %1, %0" : "=a"(ret) : "dN"(port));
	return ret;
}

static inline void insb(uint16_t port, void *addr, size_t cnt) {
	// read cnt bytes from port and put them at addr
	asm volatile ("rep insb" : "+D" (addr), "+c" (cnt) : "d" (port) : "memory");
}

static inline void insw(uint16_t port, void *addr, size_t cnt) {
	// read cnt words from port and put them at addr
	asm volatile ("rep insw" : "+D" (addr), "+c" (cnt) : "d" (port) : "memory");
}

static inline void insl(uint16_t port, void *addr, size_t cnt) {
	// read cnt longwords from port and put them at addr
	asm volatile ("rep insl" : "+D" (addr), "+c" (cnt) : "d" (port) : "memory");
}

static inline void invlpg(void* addr) {
	asm volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

#define BOCHS_BREAKPOINT asm volatile("xchg %bx, %bx")


// Utility functions for memory alignment

#define PAGE_SIZE		0x1000
#define PAGE_MASK		0xFFFFF000
#define PAGE_ALIGNED(x)     ((((size_t)(x)) & (~PAGE_MASK)) == 0)
#define PAGE_ALIGN_DOWN(x)	(((size_t)(x)) & PAGE_MASK) 
#define PAGE_ALIGN_UP(x)  	((((size_t)(x))&(~PAGE_MASK)) == 0 ? ((size_t)x) : (((size_t)x) & PAGE_MASK) + PAGE_SIZE)
#define PAGE_ID(x)			(((size_t)(x)) / PAGE_SIZE)
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


void load_kernel_symbol_table(elf_shdr_t *sym, elf_shdr_t *str);
void kernel_stacktrace(uint32_t ebp, uint32_t eip);

/* vim: set ts=4 sw=4 tw=0 noet :*/
