#include <stdbool.h>

#include <string.h>

#include <syscall.h>
#include <debug.h>

int main(int argc, char **argv) {
	dbg_print("Hello, world! from user process.\n");

	return 0;
}

