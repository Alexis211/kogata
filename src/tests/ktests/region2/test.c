
void test_region_2() {
	BEGIN_TEST("region-test-2");

	// allocate a big region and try to write into it
	const size_t n = 200;
	void* p0 = region_alloc(n * PAGE_SIZE, "Test big region", default_allocator_pf_handler);
	for (size_t i = 0; i < n; i++) {
		uint32_t *x = (uint32_t*)(p0 + i * PAGE_SIZE);
		x[0] = 12;
		x[1] = (i * 20422) % 122;
	}
	// unmap memory
	for (size_t i = 0; i < n; i++) {
		void* p = p0 + i * PAGE_SIZE;
		uint32_t *x = (uint32_t*)p;
		ASSERT(x[1] == (i * 20422) % 122);

		uint32_t f = pd_get_frame(p);
		ASSERT(f != 0);
		pd_unmap_page(p);
		ASSERT(pd_get_frame(p) == 0);

		frame_free(f, 1);
	}
	region_free(p0);

	TEST_OK;
}

#undef TEST_PLACEHOLDER_AFTER_REGION
#define TEST_PLACEHOLDER_AFTER_REGION { test_region_2(); }

/* vim: set ts=4 sw=4 tw=0 noet :*/
