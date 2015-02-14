#include <syscall.h>

extern int main(int, char**);

void __libkogata_start() {
	// TODO : setup

	int ret = main(0, 0);

	exit(ret);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
