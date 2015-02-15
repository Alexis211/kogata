#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <syscallproto.h>

#include <fs.h>
#include <debug.h>

void dbg_print(const char* str);
void yield();
void exit(int code);

// more todo

/* vim: set ts=4 sw=4 tw=0 noet :*/
