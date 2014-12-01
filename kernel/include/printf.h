#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

int snprintf(char* s, size_t n, const char* format, ...);
int vsnprintf(char* s, size_t n, const char* format, va_list arg);

/* vim: set ts=4 sw=4 tw=0 noet :*/
