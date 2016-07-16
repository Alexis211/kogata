#include <kogata/syscall.h>

#include <stdio.h>

void malloc_setup();

int main(int, char**);

void __libc_start() {
	malloc_setup();

	setup_libc_stdio();

	// TODO : more setup ? yes, for args, for env...
	char *argv[] = {"bin"};

	int ret = main(1, argv);

	sc_exit(ret);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
