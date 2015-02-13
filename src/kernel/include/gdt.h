#pragma once

#include <stddef.h>
#include <stdint.h>

/*	The GDT is one of the x86's descriptor tables. It is used for memory segmentation.
	Here, we don't use segmentation to separate processes from one another (this is done with paging).
	We only use segmentation to make the difference between kernel mode (ring 3) and user mode (ring 0) */

void gdt_init();

void set_kernel_stack(void* addr);

#define K_CODE_SEGMENT 0x08
#define K_DATA_SEGMENT 0x10
#define U_CODE_SEGMENT 0x18
#define U_DATA_SEGMENT 0x20

/* vim: set ts=4 sw=4 tw=0 noet :*/
