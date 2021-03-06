#include <string.h>

#include <kogata/malloc.h>

#include <kogata/syscall.h>
#include <kogata/debug.h>
#include <kogata/region_alloc.h>

int main(int argc, char **argv) {
	dbg_print("(BEGIN-USER-TEST malloc-test)\n");

	dbg_print_region_info();

	for (int iter = 0; iter < 4; iter++) {
		dbg_printf("Doing malloc test #%d...\n", iter + 1);
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
	}

	dbg_printf("(TEST-OK)\n");

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
