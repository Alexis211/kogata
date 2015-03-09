#pragma once

// This is in no way a complete 16-bit PC virtualizer, only a way
// to call BIOS functions.
// It is very ugly: it uses a global mutex and a lot of global variables.

#include <thread.h>
#include <mutex.h>

#define V86_ALLOC_ADDR 0x20000
#define V86_STACK_TOP 0x80000
#define V86_BIOS_BEGIN 0xA0000
#define V86_BIOS_END 0x100000

typedef uint32_t v86_farptr_t;

#define V86_SEG_OF_LIN(x)  ((size_t)(x) >> 4)
#define V86_OFF_OF_LIN(x)  ((size_t)(x) & 0x0F)
#define V86_LIN_OF_SEG_OFF(seg, off)   ((((size_t)(seg)) << 4) + ((size_t)(off)))
inline void* v86_lin_of_fp(v86_farptr_t x) {
	return (void*)V86_LIN_OF_SEG_OFF(x>>16, x & 0xFFFF);
}
inline v86_farptr_t v86_fp_of_lin(void* p) {
	return (V86_SEG_OF_LIN(p) << 16) | V86_OFF_OF_LIN(p);
}

typedef struct {
	uint16_t ax, bx, cx, dx, di, si;
	uint16_t cs, ds, es, fs, gs, ss;
	uint16_t ip, sp;
} v86_regs_t;
extern v86_regs_t v86_regs;

bool v86_begin_session();

void* v86_alloc(size_t size);

bool v86_bios_int(uint8_t int_no);

void v86_end_session();


/* vim: set ts=4 sw=4 tw=0 noet :*/
