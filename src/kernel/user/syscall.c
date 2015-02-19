#include <string.h>
#include <process.h>
#include <vfs.h>

#include <syscall.h>

typedef struct {
	uint32_t sc_id, a, b, c, d, e;		// a: ebx, b: ecx, c: edx, d: esi, e: edi
} sc_args_t;

typedef uint32_t (*syscall_handler_t)(sc_args_t);

static syscall_handler_t sc_handlers[SC_MAX] = { 0 };

static char* sc_copy_string(uint32_t s, uint32_t slen) {
	const char* addr = (const char*)s;
	probe_for_read(addr, slen);

	char* buf = malloc(slen+1);
	if (buf == 0) return 0;

	memcpy(buf, addr, slen);
	buf[slen] = 0;

	return buf;
}

// ==================== //
// THE SYSCALLS CODE !! //
// ==================== //


static uint32_t exit_sc(sc_args_t args) {
	dbg_printf("Proc %d exit with code %d\n", current_process()->pid, args.a);
	// TODO : use code... and process exiting is not supposed to be done this way

	exit();
	return 0;
}

static uint32_t yield_sc(sc_args_t args) {
	yield();
	return 0;
}

static uint32_t dbg_print_sc(sc_args_t args) {
	char* msg = sc_copy_string(args.a, args.b);
	if (msg == 0) return -1;

	dbg_print(msg);

	free(msg);
	return 0;
}

static uint32_t mmap_sc(sc_args_t args) {
	return mmap(current_process(), (void*)args.a, args.b, args.c);
}

static uint32_t mchmap_sc(sc_args_t args) {
	return mchmap(current_process(), (void*)args.a, args.b);
}

static uint32_t munmap_sc(sc_args_t args) {
	return munmap(current_process(), (void*)args.a);
}

// ====================== //
// SYSCALLS SETUP ROUTINE //
// ====================== //

void setup_syscalls() {
	sc_handlers[SC_EXIT] = exit_sc;
	sc_handlers[SC_YIELD] = yield_sc;
	sc_handlers[SC_DBG_PRINT] = dbg_print_sc;

	sc_handlers[SC_MMAP] = mmap_sc;
	sc_handlers[SC_MCHMAP] = mchmap_sc;
	sc_handlers[SC_MUNMAP] = munmap_sc;
}

void syscall_handler(registers_t *regs) {
	ASSERT(regs->int_no == 64);

	if (regs->eax < SC_MAX) {
		syscall_handler_t h = sc_handlers[regs->eax];
		if (h != 0) {
			sc_args_t args = {
				.a = regs->ebx,
				.b = regs->ecx,
				.c = regs->edx,
				.d = regs->esi,
				.e = regs->edi};
			regs->eax = h(args);
		} else {
			regs->eax = -1;
		}
	}
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
