#include <string.h>
#include <stdlib.h>

#include <proto/launch.h>

#include <kogata/debug.h>
#include <kogata/syscall.h>

int main(int argc, char **argv) {
	dbg_print("[login] Starting up.\n");

	// communication channel between terminal && shell
	fd_pair_t tc = sc_make_channel(false);

	// just launch a terminal
	pid_t term_pid = sc_new_proc();
	if (term_pid == 0) {
		PANIC("[login] Could not launch terminal");
	}

	bool ok;

	ok = sc_bind_fs(term_pid, "sys", "sys")
			&& sc_bind_fs(term_pid, "config", "config")
			&& sc_bind_fd(term_pid, STD_FD_GIP, STD_FD_GIP)
			&& sc_bind_fd(term_pid, STD_FD_TTYSRV, tc.a);
	if (!ok) PANIC("[login] Could not bind to terminal process.");

	ok = sc_proc_exec(term_pid, "sys:/bin/terminal.bin");
	if (!ok) PANIC("[login] Could not run terminal.bin");

	// and launch the shell
	pid_t shell_pid = sc_new_proc();
	if (shell_pid == 0) {
		PANIC("[login] Could not launch shell");
	}

	ok = sc_bind_fs(shell_pid, "root", "root")
			&& sc_bind_fs(shell_pid, "sys", "sys")
			&& sc_bind_fs(shell_pid, "config", "config")
			&& sc_bind_fd(shell_pid, STD_FD_TTY_STDIO, tc.b);
	if (!ok) PANIC("[login] Could not bind to shell process.");

	// ok = sc_proc_exec(shell_pid, "sys:/bin/shell.bin");
	ok = sc_proc_exec(shell_pid, "sys:/bin/lua.bin");
	if (!ok) PANIC("[login] Could not run shell.bin");

	proc_status_t s;
	sc_proc_wait(0, true, &s);

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
