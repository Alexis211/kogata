#include <string.h>
#include <process.h>
#include <vfs.h>

#include <syscall.h>

typedef uint32_t (*syscall_handler_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

static syscall_handler_t sc_handlers[SC_MAX] = { 0 };

// ==================== //
// THE SYSCALLS CODE !! //
// ==================== //

static uint32_t exit_sc(uint32_t code, uint32_t uu1, uint32_t uu2, uint32_t uu3, uint32_t uu4) {
	dbg_printf("Proc %d exit with code %d\n", proc_pid(current_process()), code);
	// TODO : use code... and process exiting is not supposed to be done this way

	exit();
	return 0;
}

static uint32_t yield_sc() {
	yield();
	return 0;
}

static uint32_t dbg_print_sc(uint32_t x, uint32_t uu1, uint32_t uu2, uint32_t uu3, uint32_t uu4) {
	const char* addr = (const char*)x;
	probe_for_read(addr, 256);

	char buf[256];
	memcpy(buf, addr, 256);
	buf[255] = 0;

	dbg_print(buf);

	return 0;
}

// ====================== //
// SYSCALLS SETUP ROUTINE //
// ====================== //

void setup_syscalls() {
	sc_handlers[SC_EXIT] = exit_sc;
	sc_handlers[SC_YIELD] = yield_sc;
	sc_handlers[SC_DBG_PRINT] = dbg_print_sc;
}

void syscall_handler(registers_t *regs) {
	ASSERT(regs->int_no == 64);

	if (regs->eax < SC_MAX) {
		syscall_handler_t h = sc_handlers[regs->eax];
		if (h != 0) {
			regs->eax = h(regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
		} else {
			regs->eax = -1;
		}
	}
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
