#include <string.h>

#include <malloc.h>

#include <syscall.h>
#include <debug.h>

int main(int argc, char **argv) {
	dbg_print("Hello, world! from user process.\n");

	fd_t f = open("io:/", FM_READDIR);
	dbg_printf("openned /: %d\n", f);
	ASSERT(f != 0);
	dirent_t x;
	size_t ent_no = 0;
	while (readdir(f, ent_no++, &x)) {
		dbg_printf("- '%s' %p %d\n", x.name, x.st.type, x.st.size);
		if (x.st.type == FT_REGULAR) {
			char buf[256];
			strcpy(buf, "io:/");
			strcpy(buf+4, x.name);
			dbg_printf("trying to open %s...\n", buf);
			fd_t ff = open(buf, FM_READ);
			ASSERT(ff != 0);

			dbg_printf("ok, open as %d\n", ff);
			char* cont = malloc(x.st.size + 1);
			ASSERT(read(ff, 0, x.st.size, cont) == x.st.size);
			cont[x.st.size] = 0;
			dbg_printf("> '%s'\n", cont);
			close(ff);
		}
	}
	close(f);

	dbg_printf("(TEST-OK)\n");

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
