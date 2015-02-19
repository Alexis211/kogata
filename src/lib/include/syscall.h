#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <syscallproto.h>
#include <mmap.h>

#include <fs.h>
#include <debug.h>

void dbg_print(const char* str);
void yield();
void exit(int code);

bool mmap(void* addr, size_t size, int mode);
bool mchmap(void* addr, int mode);
bool munmap(void* addr);

// more todo

/* vim: set ts=4 sw=4 tw=0 noet :*/
