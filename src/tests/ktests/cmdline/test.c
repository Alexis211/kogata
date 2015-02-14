
void test_cmdline(multiboot_info_t *mbd, fs_t *devfs) {
	BEGIN_TEST("test-cmdline");

	fs_handle_t *f = fs_open(devfs, "/cmdline", FM_READ);
	ASSERT(f != 0);

	char buf[256];
	size_t l = file_read(f, 0, 255, buf);
	ASSERT(l > 0);
	buf[l] = 0;

	unref_file(f);
	dbg_printf("Command line as in /cmdline file: '%s'.\n", buf);

	ASSERT(strcmp(buf, (char*)mbd->cmdline) == 0);

	TEST_OK;
}

#undef TEST_PLACEHOLDER_AFTER_DEVFS
#define TEST_PLACEHOLDER_AFTER_DEVFS { test_cmdline(mbd, devfs); }

/* vim: set ts=4 sw=4 tw=0 noet :*/
