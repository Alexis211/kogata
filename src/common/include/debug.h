#pragma once

#include <stddef.h>
#include <stdint.h>

void sys_panic(const char* message, const char* file, int line)
__attribute__((__noreturn__));

void sys_panic_assert(const char* assertion, const char* file, int line)
__attribute__((__noreturn__));

#define PANIC(s) sys_panic(s, __FILE__, __LINE__);
#define ASSERT(s) { if (!(s)) sys_panic_assert(#s, __FILE__, __LINE__); }

void dbg_print(const char* str);
void dbg_printf(const char* format, ...);

/* vim: set ts=4 sw=4 tw=0 noet :*/
