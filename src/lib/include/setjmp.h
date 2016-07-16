#pragma once

#include <stdint.h>

typedef uint32_t jmp_buf[10];

int setjmp(jmp_buf env);

void longjmp(jmp_buf env, int val);


/* vim: set sts=0 ts=4 sw=4 tw=0 noet :*/
