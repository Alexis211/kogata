#include <string.h>

#include <syscall.h>
#include <debug.h>

int main(int argc, char **argv) {
	dbg_print("Hello, world! from user process.\n");

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
