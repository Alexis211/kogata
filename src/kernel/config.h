#pragma once

#if !defined(__cplusplus)
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>
 
#if defined(__linux__)
#error "This kernel needs to be compiled with a cross-compiler."
#endif
 
#if !defined(__i386__)
#error "This kernel needs to be compiled with a ix86-elf compiler"
#endif

#define IN_KERNEL

#define K_HIGHHALF_ADDR ((size_t)0xC0000000)

#define OS_NAME "kogata"
#define OS_VERSION "0.0.1"

// Comment to disable either form of debug log output
#define DBGLOG_TO_SERIAL
#define DBGLOG_TO_SCREEN

/* vim: set ts=4 sw=4 tw=0 noet :*/
