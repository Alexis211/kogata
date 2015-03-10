#include <string.h>

#include <malloc.h>

#include <syscall.h>
#include <debug.h>
#include <user_region.h>

int main(int argc, char **argv) {
	dbg_print("[login] Starting up.\n");

	usleep(10000000);	// pretend we are doing something

	// and now pretend we have exited due to some kind of failure

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
