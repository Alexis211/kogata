#include <string.h>
#include <malloc.h>
#include <user_region.h>
#include <debug.h>

#include <stdio.h>

#include <syscall.h>

int main(int argc, char **argv) {
	dbg_printf("[shell] Starting\n");

	/*fctl(stdio, FC_SET_BLOCKING, 0);*/

	puts("Hello, world!\n");

	while(true) {
		puts("> ");
		char buf[256];
		getline(buf, 256);
		printf("You said: '%s'. I don't understand a word of that.\n\n", buf);
	}

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
