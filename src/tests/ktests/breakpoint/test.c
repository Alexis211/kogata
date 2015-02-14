
void test_breakpoint() {
	BEGIN_TEST("breakpoint-test");

	asm volatile("int $0x3");	// test breakpoint

	TEST_OK;
}

#undef TEST_PLACEHOLDER_AFTER_IDT
#define TEST_PLACEHOLDER_AFTER_IDT { test_breakpoint(); }

/* vim: set ts=4 sw=4 tw=0 noet :*/
