#include <string.h>
#include <malloc.h>
#include <user_region.h>
#include <debug.h>

#include <syscall.h>

int main(int argc, char **argv) {
	dbg_print("[login] Starting up.\n");

	// just launch a terminal
	pid_t term_pid = new_proc();
	if (term_pid == 0) {
		PANIC("[login] Could not launch terminal");
	}

	bool ok;

	ok = bind_fs(term_pid, "root", "root")
			&& bind_fs(term_pid, "sys", "sys")
			&& bind_fs(term_pid, "config", "config")
			&& bind_fd(term_pid, 1, 1);
	if (!ok) PANIC("[login] Could not bind to terminal process.");

	ok = proc_exec(term_pid, "sys:/bin/terminal.bin");
	if (!ok) PANIC("[login] Could not run terminal.bin");

	proc_status_t s;
	proc_wait(term_pid, true, &s);

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
