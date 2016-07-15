#include <kogata/syscall.h>

void malloc_setup();

int main(int, char**);

void __libkogata_start() {
	malloc_setup();

	// TODO : more setup ?

	int ret = main(0, 0);

	exit(ret);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
