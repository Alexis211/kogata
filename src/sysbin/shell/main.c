#include <string.h>
#include <stdlib.h>

#include <stdio.h>
#include <unistd.h>

#include <kogata/debug.h>
#include <kogata/syscall.h>

void ls(char* dir) {
	fd_t f = open(dir, FM_READDIR);
	if (f) {
		dirent_t i;
		int ent_no = 0;
		while (readdir(f, ent_no++, &i)) {
			if (i.st.type & FT_DIR)
				printf("%s/\n", i.name);
			else
				printf("%s\n", i.name);
		}
		close(f);
	} else {
		printf("Could not open directory '%s'\n", dir);
	}
}

void cat(char* file) {
	fd_t f = open(file, FM_READ);
	if (f) {
		char buf[129];
		size_t p = 0;
		while (true) {
			size_t r = read(f, p, 128, buf);
			p += r;
			write(stdio, 0, r, buf);
			if (r < 128) break;
		}
		close(f);
	} else {
		printf("Could not open file '%s'\n", file);
	}
}

int main(int argc, char **argv) {
	dbg_printf("[shell] Starting\n");

	fctl(stdio, FC_SET_BLOCKING, 0);

	puts("Kogata shell.\n");

	chdir("sys:");

	while(true) {
		char buf[256];
		printf("\n\e[36m%s>\e[33m ", getcwd(buf, 256));

		getline(buf, 256);

		printf("\e[39m");
		if (!strncmp(buf, "cd ", 3)) {
			chdir(buf + 3);
		} else if (!strcmp(buf, "ls")) {
			if (getcwd(buf, 256)) {
				ls(buf);
			}
		} else if (!strncmp(buf, "ls ", 3)) {
			char buf2[256];
			if (getcwd(buf2, 256)) {
				if (pathncat(buf2, buf + 3, 256)) {
					ls(buf2);
				}
			}
		} else if (!strncmp(buf, "cat ", 4)) {
			char buf2[256];
			if (getcwd(buf2, 256)) {
				if (pathncat(buf2, buf+4, 256)) {
					cat(buf2);
				}
			}
		} else if (!strcmp(buf, "exit")) {
			break;
		} else {
			printf("No such command.\n");
		}
	}
	printf("Bye.\n");

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
