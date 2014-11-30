#include <multiboot.h>
#include <config.h>
#include <dbglog.h>
#include <sys.h>
 
void kmain(struct multiboot_info_t *mbd, int32_t mb_magic) {
	dbglog_setup();

	dbg_printf("Hello, kernel World!\n");
	dbg_printf("This is %s, version %s.\n", OS_NAME, OS_VERSION);

	PANIC("Reached kmain end! Falling off the edge.");
}

