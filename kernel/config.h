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


extern char k_highhalf_addr;	// defined in linker script : 0xC0000000
#define K_HIGHHALF_ADDR ((void*)&k_highhalf_addr)

#define OS_NAME "Macroscope"
#define OS_VERSION "0.0.1"

// Comment to disable either form of debug log output
#define DBGLOG_TO_SERIAL
#define DBGLOG_TO_SCREEN


/* vim: set ts=4 sw=4 tw=0 noet :*/
