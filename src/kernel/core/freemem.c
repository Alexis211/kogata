#include <freemem.h>
#include <thread.h>
#include <debug.h>
#include <frame.h>

void free_some_memory() {
	dbg_print_frame_stats();
	dbg_printf("Currently out of memory ; free_some_memory not implemented. Waiting.\n");
	usleep(1000000);	// Hope someone will do something...
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
