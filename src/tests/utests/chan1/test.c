#include <string.h>

#include <malloc.h>

#include <syscall.h>
#include <debug.h>

int main(int argc, char **argv) {

	fd_pair_t ch = make_channel(false);

	char* s = "Hello, world!";
	ASSERT(write(ch.a, 0, strlen(s), s) == strlen(s));
	char buf[128];
	ASSERT(read(ch.b, 0, 128, buf) == strlen(s));
	ASSERT(strncmp(s, buf, strlen(s)) == 0);

	close(ch.a);
	sel_fd_t sel = { .fd = ch.b, .req_flags = SEL_ERROR };
	ASSERT(select(&sel, 1, 0));
	ASSERT(sel.got_flags & SEL_ERROR);
	
	close(ch.b);

	dbg_printf("(TEST-OK)\n");

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
