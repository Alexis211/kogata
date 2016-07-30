#include <string.h>

#include <kogata/malloc.h>

#include <kogata/syscall.h>
#include <kogata/debug.h>

volatile int step = 0;		// volatile because thread-shared

const char* msg_a = "ping";
const char* msg_b = "potato salad";
const char* msg_c = "what?";
const char* msg_d = "hello world";

void listen_thread(fd_t c1, fd_t c2) {
	dbg_printf("listen_thread started\n");

	dbg_printf("Got args: %d, %d\n", c1, c2);

	sel_fd_t sel[2];
	sel[0].fd = c1;
	sel[1].fd = c2;
	sel[0].req_flags = sel[1].req_flags = SEL_READ | SEL_ERROR;

	char buf[128];

	dbg_printf("Doing first select...\n");
	ASSERT(sc_select(sel, 2, -1));
	dbg_printf("%x %x\n", sel[0].got_flags, sel[1].got_flags);
	ASSERT(sel[0].got_flags & SEL_READ);
	ASSERT(!(sel[1].got_flags & SEL_READ));
	ASSERT(sc_read(c1, 0, strlen(msg_a), buf) == strlen(msg_a));
	ASSERT(strncmp(buf, msg_a, strlen(msg_a)) == 0);
	ASSERT(sc_write(c1, 0, strlen(msg_c), msg_c) == strlen(msg_c));
	step = 1;

	dbg_printf("Doing second select...\n");
	ASSERT(sc_select(sel, 2, -1));
	dbg_printf("%x %x\n", sel[0].got_flags, sel[1].got_flags);
	ASSERT(sel[1].got_flags & SEL_READ);
	ASSERT(!(sel[0].got_flags & SEL_READ));
	ASSERT(sc_read(c2, 0, strlen(msg_b), buf) == strlen(msg_b));
	ASSERT(strncmp(buf, msg_b, strlen(msg_b)) == 0);
	ASSERT(sc_write(c2, 0, strlen(msg_d), msg_d) == strlen(msg_d));
	step = 2;

	dbg_printf("Doing third select...\n");
	ASSERT(sc_select(sel, 2, -1));
	dbg_printf("%x %x\n", sel[0].got_flags, sel[1].got_flags);
	ASSERT(sel[0].got_flags & SEL_ERROR);
	ASSERT(!(sel[1].got_flags & SEL_ERROR));
	sc_close(c1);
	step = 3;

	dbg_printf("Doing fourth select...\n");
	ASSERT(sc_select(sel + 1, 1, -1));
	dbg_printf("%x\n", sel[1].got_flags);
	ASSERT(sel[1].got_flags & SEL_ERROR);
	sc_close(c2);
	step = 4;

	sc_exit_thread();
}

int main(int argc, char **argv) {

	fd_pair_t ch1 = sc_make_channel(false);
	ASSERT(ch1.a != 0 && ch1.b != 0);

	fd_pair_t ch2 = sc_make_channel(false);
	ASSERT(ch2.a != 0 && ch2.b != 0);

	void* stack = malloc(0x1000);
	ASSERT(stack != 0);

	char buf[128];

	uint32_t *esp = (uint32_t*)(stack + 0x1000);
	*(--esp) = ch2.a;
	*(--esp) = ch1.a;
	*(--esp) = 0;	// false return address
	dbg_printf("launching waiter thread (args: %d, %d)\n", ch1.a, ch2.a);
	ASSERT(sc_sys_new_thread(listen_thread, esp));

	fd_t c1 = ch1.b, c2 = ch2.b;

	// -- the test on the first channel

	ASSERT(sc_write(c1, 0, strlen(msg_a), msg_a) == strlen(msg_a));

	dbg_printf("wait for step1 signal\n");
	while(step != 1);
	dbg_printf("got step1 signal, proceeding\n");

	ASSERT(sc_read(c1, 0, strlen(msg_c), buf) == strlen(msg_c));
	ASSERT(strncmp(msg_c, buf, strlen(msg_c)) == 0);

	// -- the test on the second channel

	ASSERT(sc_write(c2, 0, strlen(msg_b), msg_b) == strlen(msg_b));

	dbg_printf("wait for step2 signal\n");
	while(step != 2);
	dbg_printf("got step2 signal, proceeding\n");

	ASSERT(sc_read(c2, 0, strlen(msg_d), buf) == strlen(msg_d));
	ASSERT(strncmp(msg_d, buf, strlen(msg_d)) == 0);

	// -- closing stuff

	sc_close(c1);

	dbg_printf("waiting for step3 signal\n");
	while(step != 3);
	dbg_printf("got step3 signal, proceeding\n");

	sc_close(c2);

	dbg_printf("waiting for step4 signal\n");
	while(step != 4);
	dbg_printf("got step4 signal, proceeding\n");

	dbg_printf("(TEST-OK)\n");

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
