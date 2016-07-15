#pragma once

// TODO

struct _jmp_buf {
	// TODO
	int a;
};
typedef struct _jmp_buf jmp_buf;

int setjmp(jmp_buf env);
//int sigsetjmp(sigjmp_buf env, int savesigs);

void longjmp(jmp_buf env, int val);
//void siglongjmp(sigjmp_buf env, int val);



/* vim: set sts=0 ts=4 sw=4 tw=0 noet :*/
