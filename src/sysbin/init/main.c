#include <string.h>
#include <stdlib.h>

#include <proto/launch.h>

#include <kogata/syscall.h>
#include <kogata/debug.h>
#include <kogata/printf.h>

#include <kogata/btree.h>

pid_t giosrv_pid = 0, login_pid = 0;
fd_pair_t root_gip_chan;

void _parse_cmdline_iter(void* a, void* b) {
	dbg_printf("  '%s'  ->  '%s'\n", a, b);
}

btree_t *parse_cmdline(const char* x) {
	btree_t *ret = create_btree(str_key_cmp_fun, free_key_val);
	ASSERT(ret != 0);

	x = strchr(x, ' ');
	if (x == 0) return ret;

	while ((*x) != 0) {
		while (*x == ' ') x++;

		char* eq = strchr(x, '=');
		char* sep = strchr(x, ' ');
		if (sep == 0) {
			if (eq == 0) {
				btree_add(ret, strdup(x), strdup(x));
			} else {
				btree_add(ret, strndup(x, eq - x), strdup(eq + 1));
			}
			break;
		} else {
			if (eq == 0 || eq > sep) {
				btree_add(ret, strndup(x, sep - x), strndup(x, sep - x));
			} else {
				btree_add(ret, strndup(x, eq - x), strndup(eq + 1, sep - eq - 1));
			}
			x = sep + 1;
		}
	}

	btree_iter(ret, _parse_cmdline_iter);
	
	return ret;
}

btree_t* read_cmdline() {
	char cmdline_buf[256];

	fd_t f = sc_open("io:/cmdline", FM_READ);
	if (f == 0) return 0;

	size_t len = sc_read(f, 0, 255, cmdline_buf);
	cmdline_buf[len] = 0;

	sc_close(f);

	return parse_cmdline(cmdline_buf);
}

void setup_sys() {
	fd_t sysdir_cfg = sc_open("config:/sysdir", FM_READ);
	if (sysdir_cfg == 0) PANIC("[init] Could not read config:/sysdir");

	char buf[256];
	size_t l = sc_read(sysdir_cfg, 0, 255, buf);
	buf[l] = 0;
	sc_close(sysdir_cfg);

	char* eol = strchr(buf, '\n');
	if (eol) *eol = 0;

	dbg_printf("[init] Using system directory %s\n", buf);
	bool ok;

	char* sep = strchr(buf, ':');
	if (sep == 0) {
		ok = sc_fs_subfs("sys", "root", buf, FM_READ | FM_MMAP | FM_READDIR);
	} else {
		*sep = 0;
		ok = sc_fs_subfs("sys", buf, sep +1, FM_READ | FM_MMAP | FM_READDIR);
	}

	if (!ok) PANIC("[init] Could not bind root:/sys to sys:/");
}

void launch_giosrv() {
	if (giosrv_pid != 0) return;

	giosrv_pid = sc_new_proc();
	if (giosrv_pid == 0) {
		PANIC("[init] Could not create process for giosrv");
	}

	dbg_printf("[init] Setting up giosrv, pid: %d\n", giosrv_pid);

	bool ok;

	ok = sc_bind_fs(giosrv_pid, "io", "io");
	if (!ok) PANIC("[init] Could not bind io:/ to giosrv");

	ok = sc_bind_fs(giosrv_pid, "sys", "sys");
	if (!ok) PANIC("[init] Could not bind sys:/ to giosrv");

	ok = sc_bind_fs(giosrv_pid, "config", "config");
	if (!ok) PANIC("[init] Could not bind config:/ to giosrv");

	ok = sc_bind_fd(giosrv_pid, STD_FD_GIOSRV, root_gip_chan.a);
	if (!ok) PANIC("[init] Could not bind root GIP channel FD to giosrv");

	ok = sc_proc_exec(giosrv_pid, "sys:/bin/giosrv.bin");
	if (!ok) PANIC("[init] Could not run giosrv.bin");

	dbg_printf("[init] giosrv started.\n");
}

void launch_login() {
	if (login_pid != 0) return;

	login_pid = sc_new_proc();
	if (login_pid == 0) {
		PANIC("[init] Could not create process for login");
	}

	dbg_printf("[init] Setting up login, pid: %d\n", login_pid);

	bool ok;

	ok = sc_bind_fs(login_pid, "root", "root");
	if (!ok) PANIC("[init] Could not bind root:/ to login");

	ok = sc_bind_fs(login_pid, "sys", "sys");
	if (!ok) PANIC("[init] Could not bind sys:/ to login");

	ok = sc_bind_fs(login_pid, "config", "config");
	if (!ok) PANIC("[init] Could not bind config:/ to login");

	ok = sc_bind_fd(login_pid, STD_FD_GIP, root_gip_chan.b);
	if (!ok) PANIC("[init] Could not bind root GIP channel FD to login");

	ok = sc_proc_exec(login_pid, "sys:/bin/login.bin");
	if (!ok) PANIC("[init] Could not run login.bin");

	dbg_printf("[init] login started.\n");
}

int main(int argc, char **argv) {
	dbg_print("[init] Starting up!\n");

	// Read kernel cmdline
	btree_t *cmdline = read_cmdline();
	if (cmdline == 0) PANIC("[init] Could not parse cmdline");

	// setup config:
	if (btree_find(cmdline, "config") == 0) {
		PANIC("[init] No config=xxx option specified on command line");
	} else {
		char* config = (char*)btree_find(cmdline, "config");
		dbg_printf("[init] Loading system configuration: %s\n", config);
		ASSERT(strlen(config) < 30);

		char buf[50];
		snprintf(buf, 50, "/config/%s", config);
		bool ok = sc_fs_subfs("config", "root", buf, FM_READ | FM_WRITE | FM_MMAP | FM_READDIR);
		if (!ok) PANIC("[init] Could not setup config:");
	}

	// Setup sys:
	setup_sys();

	// Setup GIP channel for communication between giosrv and login
	root_gip_chan = sc_make_channel(false);
	if (root_gip_chan.a == 0 || root_gip_chan.b == 0) {
		PANIC("[init] Could not create root GIP channel.");
	}

	// Launch giosrv && login
	launch_giosrv();
	launch_login();

	// Make sure no one dies
	while(true) {
		proc_status_t s;
		sc_proc_wait(0, false, &s);
		if (s.pid != 0) {
			if (s.pid == giosrv_pid) {
				giosrv_pid = 0;

				dbg_printf("[init] giosrv died, restarting.\n");
				launch_giosrv();
			} else if (s.pid == login_pid) {
				login_pid = 0;

				dbg_printf("[init] login died, restarting.\n");
				launch_login();
			} else {
				ASSERT(false);
			}
		}
		sc_usleep(1000000);
	}

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
