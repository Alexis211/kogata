#include <string.h>

#include <kogata/malloc.h>

#include <kogata/syscall.h>
#include <kogata/debug.h>

int main(int argc, char **argv) {

	fd_pair_t ch = sc_make_channel(false);

	char* s = "Hello, world!";
	ASSERT(sc_write(ch.a, 0, strlen(s), s) == strlen(s));
	char buf[128];
	ASSERT(sc_read(ch.b, 0, 128, buf) == strlen(s));
	ASSERT(strncmp(s, buf, strlen(s)) == 0);

	sc_close(ch.a);
	sel_fd_t sel = { .fd = ch.b, .req_flags = SEL_ERROR };
	ASSERT(sc_select(&sel, 1, 0));
	ASSERT(sel.got_flags & SEL_ERROR);
	
	sc_close(ch.b);

	dbg_printf("(TEST-OK)\n");

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
