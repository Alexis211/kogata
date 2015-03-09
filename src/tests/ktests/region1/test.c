
void test_region_1() {
	BEGIN_TEST("region-test-1");

	void* p = region_alloc(0x1000, "Test region");
	dbg_printf("Allocated one-page region: 0x%p\n", p);
	dbg_print_region_info();
	void* q = region_alloc(0x1000, "Test region");
	dbg_printf("Allocated one-page region: 0x%p\n", q);
	dbg_print_region_info();
	void* r = region_alloc(0x2000, "Test region");
	dbg_printf("Allocated two-page region: 0x%p\n", r);
	dbg_print_region_info();
	void* s = region_alloc(0x10000, "Test region");
	dbg_printf("Allocated 16-page region: 0x%p\n", s);
	dbg_print_region_info();
	region_free(p);
	dbg_printf("Freed region 0x%p\n", p);
	dbg_print_region_info();
	region_free(q);
	dbg_printf("Freed region 0x%p\n", q);
	dbg_print_region_info();
	region_free(r);
	dbg_printf("Freed region 0x%p\n", r);
	dbg_print_region_info();
	region_free(s);
	dbg_printf("Freed region 0x%p\n", s);
	dbg_print_region_info();

	TEST_OK;
}

#undef TEST_PLACEHOLDER_AFTER_REGION
#define TEST_PLACEHOLDER_AFTER_REGION { test_region_1(); }

/* vim: set ts=4 sw=4 tw=0 noet :*/
