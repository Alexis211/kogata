#pragma once

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

void putchar(int c);
void puts(char* s);
void printf(char* arg, ...);

int getchar();
void getline(char* buf, size_t l);

/* vim: set ts=4 sw=4 tw=0 noet :*/
