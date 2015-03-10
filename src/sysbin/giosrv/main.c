#include <string.h>

#include <malloc.h>

#include <syscall.h>
#include <debug.h>
#include <user_region.h>

#include <proto/fb.h>

int main(int argc, char **argv) {
	dbg_print("[giosrv] Starting up.\n");

	fd_t fbdev = open("io:/display/vesa", FM_READ | FM_WRITE | FM_MMAP);
	if (fbdev == 0) PANIC("Could not open fbdev");

	framebuffer_info_t i;
	int r = ioctl(fbdev, IOCTL_FB_GET_INFO, &i);
	dbg_printf("[giosrv] ioctl -> %d\n", r);
	ASSERT(r == 1);
	dbg_printf("[giosrv] Running on FB %dx%d\n", i.width, i.height);

	while(true);	// nothing to do

	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
