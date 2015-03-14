#pragma once

#include <stdarg.h>

#include <syscall.h>

extern fd_t stdio;

void putchar(int c);
void puts(char* s);
void printf(char* arg, ...);

int getchar();
void getline(char* buf, size_t l);

/* vim: set ts=4 sw=4 tw=0 noet :*/
