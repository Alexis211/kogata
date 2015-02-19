#include <string.h>

#include <malloc.h>

#include <syscall.h>
#include <debug.h>
#include <user_region.h>

int main(int argc, char **argv) {
	dbg_print("Hello, world! from user process.\n");

	dbg_print_region_info();

	dbg_printf("Doing malloc test...\n");
	const int m = 200;
	uint16_t** ptr = malloc(m * sizeof(uint32_t));
	for (int i = 0; i < m; i++) {
		size_t s = 1 << ((i * 7) % 11 + 2);
		ptr[i] = (uint16_t*)malloc(s);
		ASSERT((size_t)ptr[i] >= 0x40000000 && (size_t)ptr[i] < 0xB0000000);
		*ptr[i] = ((i * 211) % 1024);
	}
	dbg_printf("Fully allocated.\n");
	dbg_print_region_info();
	for (int i = 0; i < m; i++) {
		for (int j = i; j < m; j++) {
			ASSERT(*ptr[j] == (j * 211) % 1024);
		}
		free(ptr[i]);
	}
	free(ptr);
	dbg_printf("malloc test OK.\n");
	dbg_print_region_info();

	fd_t f = open("dev:/", FM_READDIR);
	dbg_printf("openned /: %d\n", f);
	dirent_t x;
	while (readdir(f, &x)) {
		dbg_printf("- '%s' %p %d\n", x.name, x.st.type, x.st.size);
		if (x.st.type == FT_REGULAR) {
			char buf[256];
			strcpy(buf, "dev:/");
			strcpy(buf+5, x.name);
			dbg_printf("trying to open %s...\n", buf);
			fd_t ff = open(buf, FM_READ);
			if (ff != 0) {
				dbg_printf("ok, open as %d\n", ff);
				char* cont = malloc(x.st.size + 1);
				read(ff, 0, x.st.size, cont);
				cont[x.st.size] = 0;
				dbg_printf("   %s\n", cont);
				close(ff);
			} else {
				dbg_printf("Could not open '%s'\n", buf);
			}
		}
	}
	close(f);

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
