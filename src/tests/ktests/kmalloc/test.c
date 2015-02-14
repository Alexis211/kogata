
void kmalloc_test(void* kernel_data_end) {
	BEGIN_TEST("kmalloc-test");

	dbg_print_region_info();
	const int m = 200;
	uint16_t** ptr = malloc(m * sizeof(uint32_t));
	for (int i = 0; i < m; i++) {
		size_t s = 1 << ((i * 7) % 11 + 2);
		ptr[i] = (uint16_t*)malloc(s);
		ASSERT((void*)ptr[i] >= kernel_data_end && (size_t)ptr[i] < 0xFFC00000);
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
	dbg_printf("Kmalloc test OK.\n");
	dbg_print_region_info();

	TEST_OK;
}

#undef TEST_PLACEHOLDER_AFTER_KMALLOC
#define TEST_PLACEHOLDER_AFTER_KMALLOC { kmalloc_test(kernel_data_end); }

/* vim: set ts=4 sw=4 tw=0 noet :*/
