#pragma once

#include <stdlib.h>
#include <stdarg.h>

int snprintf(char* s, size_t n, const char* format, ...);
int vsnprintf(char* s, size_t n, const char* format, va_list arg);

