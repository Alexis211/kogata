#include <string.h>

#include <malloc.h>

#include <syscall.h>
#include <debug.h>

int main(int argc, char **argv) {
	dbg_print("Hello, world! from user process.\n");

	fd_t f = open("io:/mod", FM_READDIR);
	dbg_printf("openned io:/mod as %d\n", f);
	ASSERT(f != 0);

	dirent_t x;
	size_t ent_no = 0;
	while (ent_no < 2) {
		ASSERT (readdir(f, ent_no++, &x));

		ASSERT((!strcmp(x.name, "utest_fs2.bin")) || (!strcmp(x.name, "kernel.map")));
		ASSERT(x.st.type == FT_REGULAR);
	}

	ASSERT(!readdir(f, ent_no++, &x));
	close(f);

	dbg_printf("(TEST-OK)\n");

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
