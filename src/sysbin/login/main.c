#include <string.h>
#include <stdlib.h>

#include <kogata/debug.h>
#include <kogata/syscall.h>

int main(int argc, char **argv) {
	dbg_print("[login] Starting up.\n");

	// communication channel between terminal && shell
	fd_pair_t tc = make_channel(false);

	// just launch a terminal
	pid_t term_pid = new_proc();
	if (term_pid == 0) {
		PANIC("[login] Could not launch terminal");
	}

	bool ok;

	ok = bind_fs(term_pid, "sys", "sys")
			&& bind_fs(term_pid, "config", "config")
			&& bind_fd(term_pid, 1, 1)
			&& bind_fd(term_pid, 2, tc.a);
	if (!ok) PANIC("[login] Could not bind to terminal process.");

	ok = proc_exec(term_pid, "sys:/bin/terminal.bin");
	if (!ok) PANIC("[login] Could not run terminal.bin");

	// and launch the shell
	pid_t shell_pid = new_proc();
	if (shell_pid == 0) {
		PANIC("[login] Could not launch shell");
	}

	ok = bind_fs(shell_pid, "root", "root")
			&& bind_fs(shell_pid, "sys", "sys")
			&& bind_fs(shell_pid, "config", "config")
			&& bind_fd(shell_pid, 1, tc.b);
	if (!ok) PANIC("[login] Could not bind to shell process.");

	ok = proc_exec(shell_pid, "sys:/bin/shell.bin");
	if (!ok) PANIC("[login] Could not run shell.bin");

	proc_status_t s;
	proc_wait(0, true, &s);

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
