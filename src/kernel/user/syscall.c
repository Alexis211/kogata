#include <string.h>
#include <process.h>
#include <vfs.h>

#include <sct.h>

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

static uint32_t usleep_sc(sc_args_t args) {
	usleep(args.a);
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

static uint32_t mmap_file_sc(sc_args_t args) {
	int fd = args.a;
	fs_handle_t *h = proc_read_fd(current_process(), fd);
	if (h == 0) return false;

	return mmap_file(current_process(), h, args.b, (void*)args.c, args.d, args.e);
}

static uint32_t mchmap_sc(sc_args_t args) {
	return mchmap(current_process(), (void*)args.a, args.b);
}

static uint32_t munmap_sc(sc_args_t args) {
	return munmap(current_process(), (void*)args.a);
}

static uint32_t create_sc(sc_args_t args) {
	bool ret = false;

	char* fn = sc_copy_string(args.a, args.b);
	if (fn == 0) goto end_create;

	char* sep = strchr(fn, ':');
	if (sep == 0) goto end_create;

	*sep = 0;
	char* file = sep + 1;

	fs_t *fs = proc_find_fs(current_process(), fn);
	if (fs == 0) goto end_create;

	ret = fs_create(fs, file, args.c);
	
end_create:
	if (fn) free(fn);
	return ret;
}

static uint32_t delete_sc(sc_args_t args) {
	bool ret = false;

	char* fn = sc_copy_string(args.a, args.b);
	if (fn == 0) goto end_del;

	char* sep = strchr(fn, ':');
	if (sep == 0) goto end_del;

	*sep = 0;
	char* file = sep + 1;

	fs_t *fs = proc_find_fs(current_process(), fn);
	if (fs == 0) goto end_del;

	ret = fs_delete(fs, file);
	
end_del:
	if (fn) free(fn);
	return ret;
}

static uint32_t move_sc(sc_args_t args) {
	bool ret = false;

	char *fn_a = sc_copy_string(args.a, args.b),
		 *fn_b = sc_copy_string(args.c, args.d);
	if (fn_a == 0 || fn_b == 0) goto end_move;

	char* sep_a = strchr(fn_a, ':');
	if (sep_a == 0) goto end_move;
	*sep_a = 0;

	char* sep_b = strchr(fn_b, ':');
	if (sep_b == 0) goto end_move;
	*sep_b = 0;

	if (strcmp(fn_a, fn_b) != 0) goto end_move;		// can only move within same FS

	char *file_a = sep_a + 1, *file_b = sep_b + 1;

	fs_t *fs = proc_find_fs(current_process(), fn_a);
	if (fs == 0) goto end_move;

	ret = fs_move(fs, file_a, file_b);
	
end_move:
	if (fn_a) free(fn_a);
	if (fn_b) free(fn_b);
	return ret;
}

static uint32_t stat_sc(sc_args_t args) {
	bool ret = false;

	char* fn = sc_copy_string(args.a, args.b);
	if (fn == 0) goto end_stat;

	char* sep = strchr(fn, ':');
	if (sep == 0) goto end_stat;

	*sep = 0;
	char* file = sep + 1;

	fs_t *fs = proc_find_fs(current_process(), fn);
	if (fs == 0) goto end_stat;

	probe_for_write((stat_t*)args.c, sizeof(stat_t));
	ret = fs_stat(fs, file, (stat_t*)args.c);
	
end_stat:
	if (fn) free(fn);
	return ret;
}

static uint32_t open_sc(sc_args_t args) {
	int ret = 0;

	char* fn = sc_copy_string(args.a, args.b);
	if (fn == 0) goto end_open;

	char* sep = strchr(fn, ':');
	if (sep == 0) goto end_open;

	*sep = 0;
	char* file = sep + 1;

	fs_t *fs = proc_find_fs(current_process(), fn);
	if (fs == 0) goto end_open;

	fs_handle_t *h = fs_open(fs, file, args.c);
	if (h == 0) goto end_open;

	ret = proc_add_fd(current_process(), h);
	if (ret == 0) unref_file(h);
	
end_open:
	if (fn) free(fn);
	return ret;
}

static uint32_t close_sc(sc_args_t args) {
	proc_close_fd(current_process(), args.a);
	return 0;
}

static uint32_t read_sc(sc_args_t args) {
	fs_handle_t *h = proc_read_fd(current_process(), args.a);
	if (h == 0) return 0;

	char* data = (char*)args.d;
	size_t len = args.c;
	probe_for_write(data, len);
	return file_read(h, args.b, len, data);
}

static uint32_t write_sc(sc_args_t args) {
	fs_handle_t *h = proc_read_fd(current_process(), args.a);
	if (h == 0) return 0;

	char* data = (char*)args.d;
	size_t len = args.c;
	probe_for_read(data, len);
	return file_write(h, args.b, len, data);
}

static uint32_t readdir_sc(sc_args_t args) {
	fs_handle_t *h = proc_read_fd(current_process(), args.a);
	if (h == 0) return false;

	dirent_t *o = (dirent_t*)args.b;
	probe_for_write(o, sizeof(dirent_t));
	return file_readdir(h, o);
}

static uint32_t stat_open_sc(sc_args_t args) {
	fs_handle_t *h = proc_read_fd(current_process(), args.a);
	if (h == 0) return false;

	stat_t *o = (stat_t*)args.b;
	probe_for_write(o, sizeof(stat_t));
	return file_stat(h, o);
}

static uint32_t ioctl_sc(sc_args_t args) {
	fs_handle_t *h = proc_read_fd(current_process(), args.a);
	if (h == 0) return -1;

	void* data = (void*)args.c;
	if (data >= (void*)K_HIGHHALF_ADDR) return -1;
	return file_ioctl(h, args.b, data);
}

static uint32_t get_mode_sc(sc_args_t args) {
	fs_handle_t *h = proc_read_fd(current_process(), args.a);
	if (h == 0) return 0;

	return file_get_mode(h);
}

// ====================== //
// SYSCALLS SETUP ROUTINE //
// ====================== //

void setup_syscall_table() {
	sc_handlers[SC_EXIT] = exit_sc;
	sc_handlers[SC_YIELD] = yield_sc;
	sc_handlers[SC_DBG_PRINT] = dbg_print_sc;
	sc_handlers[SC_USLEEP] = usleep_sc;

	sc_handlers[SC_MMAP] = mmap_sc;
	sc_handlers[SC_MMAP_FILE] = mmap_file_sc;
	sc_handlers[SC_MCHMAP] = mchmap_sc;
	sc_handlers[SC_MUNMAP] = munmap_sc;

	sc_handlers[SC_CREATE] = create_sc;
	sc_handlers[SC_DELETE] = delete_sc;
	sc_handlers[SC_MOVE] = move_sc;
	sc_handlers[SC_STAT] = stat_sc;

	sc_handlers[SC_OPEN] = open_sc;
	sc_handlers[SC_CLOSE] = close_sc;
	sc_handlers[SC_READ] = read_sc;
	sc_handlers[SC_WRITE] = write_sc;
	sc_handlers[SC_READDIR] = readdir_sc;
	sc_handlers[SC_STAT_OPEN] = stat_open_sc;
	sc_handlers[SC_IOCTL] = ioctl_sc;
	sc_handlers[SC_GET_MODE] = get_mode_sc;
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
