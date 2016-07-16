#pragma once

#include <stdint.h>

struct _jmp_buf {
	uint32_t stuff[10];	// 40 bytes
};
typedef struct _jmp_buf jmp_buf;

int setjmp(jmp_buf env);

void longjmp(jmp_buf env, int val);


/* vim: set sts=0 ts=4 sw=4 tw=0 noet :*/
