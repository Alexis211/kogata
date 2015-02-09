#pragma once

#include <stddef.h>
#include <stdint.h>

void panic(const char* message, const char* file, int line);
void panic_assert(const char* assertion, const char* file, int line);
#define PANIC(s) panic(s, __FILE__, __LINE__);
#define ASSERT(s) { if (!(s)) panic_assert(#s, __FILE__, __LINE__); }

void dbg_print(const char* str);
void dbg_printf(const char* format, ...);

/* vim: set ts=4 sw=4 tw=0 noet :*/
