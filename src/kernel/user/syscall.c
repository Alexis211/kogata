#include <string.h>
#include <process.h>
#include <vfs.h>
#include <elf.h>

#include <sct.h>

typedef struct {
	uint32_t sc_id, a, b, c, d, e;		// a: ebx, b: ecx, c: edx, d: esi, e: edi
} sc_args_t;

typedef uint32_t (*syscall_handler_t)(sc_args_t);

static syscall_handler_t sc_handlers[SC_MAX] = { 0 };

static char* sc_copy_string(const char* addr, size_t slen) {
	probe_for_read(addr, slen);

	char* buf = malloc(slen+1);
	if (buf == 0) return 0;

	memcpy(buf, addr, slen);
	buf[slen] = 0;

	return buf;
}

static char* sc_copy_string_x(uint32_t s, uint32_t slen) {
	return sc_copy_string((const char*)s, slen);
}

// ==================== //
// THE SYSCALLS CODE !! //
// ==================== //

//  ---- Related to the current process's execution

static uint32_t exit_sc(sc_args_t args) {
	dbg_printf("Proc %d exit with code %d\n", current_process()->pid, args.a);

	current_process_exit(PS_FINISHED, args.a);
	ASSERT(false);
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
	char* msg = sc_copy_string_x(args.a, args.b);
	if (msg == 0) return -1;

	dbg_print(msg);

	free(msg);
	return 0;
}

//  ---- Memory management related

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

//  ---- Accessing the VFS - filesystems

static uint32_t create_sc(sc_args_t args) {
	bool ret = false;

	char* fn = sc_copy_string_x(args.a, args.b);
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

	char* fn = sc_copy_string_x(args.a, args.b);
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

	char *fn_a = sc_copy_string_x(args.a, args.b),
		 *fn_b = sc_copy_string_x(args.c, args.d);
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

	char* fn = sc_copy_string_x(args.a, args.b);
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

//  ---- Accessing the VFS - files

static uint32_t open_sc(sc_args_t args) {
	int ret = 0;

	char* fn = sc_copy_string_x(args.a, args.b);
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

	dirent_t *o = (dirent_t*)args.c;
	probe_for_write(o, sizeof(dirent_t));
	return file_readdir(h, args.b, o);
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

//  ---- Managing file systems

static uint32_t make_fs_sc(sc_args_t args) {
	sc_make_fs_args_t *a = (sc_make_fs_args_t*)args.a;
	probe_for_read(a, sizeof(sc_make_fs_args_t));

	bool ok = false;

	char* driver = 0;
	char* fs_name = 0;
	char* opts = 0;
	fs_t *the_fs = 0;

	process_t *p;
	if (a->bind_to_pid == 0) {
		p = current_process();
	} else {
		p = process_find_child(current_process(), a->bind_to_pid);
		if (p == 0) goto end_mk_fs;
	}

	fs_handle_t *source = 0;
	if (a->source_fd != 0) {
		source = proc_read_fd(current_process(), a->source_fd);
		if (!source) goto end_mk_fs;
	}

	driver = sc_copy_string(a->driver, a->driver_strlen);
	if (!driver) goto end_mk_fs;

	fs_name = sc_copy_string(a->fs_name, a->fs_name_strlen);
	if (!fs_name) goto end_mk_fs;

	opts = sc_copy_string(a->opts, a->opts_strlen);
	if (!opts) goto end_mk_fs;
	
	the_fs = make_fs(driver, source, opts);
	if (!the_fs) goto end_mk_fs;

	ok = proc_add_fs(p, the_fs, fs_name);

end_mk_fs:
	if (driver) free(driver);
	if (fs_name) free(fs_name);
	if (opts) free(opts);
	if (the_fs && !ok) unref_fs(the_fs);
	return ok;
}

static uint32_t fs_add_src_sc(sc_args_t args) {
	bool ok = false;

	char* opts = 0;
	char* fs_name = 0;

	fs_name = sc_copy_string_x(args.a, args.b);
	if (fs_name == 0) goto end_add_src;

	opts = sc_copy_string_x(args.d, args.e);
	if (opts == 0) goto end_add_src;

	fs_t *fs = proc_find_fs(current_process(), fs_name);
	if (fs == 0) goto end_add_src;

	fs_handle_t *src = proc_read_fd(current_process(), args.c);
	if (src == 0) goto end_add_src;

	ok = fs_add_source(fs, src, opts);

end_add_src:
	if (fs_name) free(fs_name);
	if (opts) free(opts);
	return ok;
}

static uint32_t fs_subfs_sc(sc_args_t args) {
	sc_subfs_args_t *a = (sc_subfs_args_t*)args.a;
	probe_for_read(a, sizeof(sc_subfs_args_t));

	bool ok = false;

	char* new_name = 0;
	char* from_fs = 0;
	char* root = 0;
	fs_t* new_fs = 0;

	process_t *p;
	if (a->bind_to_pid == 0) {
		p = current_process();
	} else {
		p = process_find_child(current_process(), a->bind_to_pid);
		if (p == 0) goto end_subfs;
	}

	new_name = sc_copy_string(a->new_name, a->new_name_strlen);
	if (!new_name) goto end_subfs;

	from_fs = sc_copy_string(a->from_fs, a->from_fs_strlen);
	if (!from_fs) goto end_subfs;

	root = sc_copy_string(a->root, a->root_strlen);
	if (!root) goto end_subfs;

	fs_t *orig_fs = proc_find_fs(current_process(), from_fs);
	if (!orig_fs) goto end_subfs;

	new_fs = fs_subfs(orig_fs, root, a->ok_modes);
	if (!new_fs) goto end_subfs;

	ok = proc_add_fs(p, new_fs, new_name);

end_subfs:
	if (new_name) free(new_name);
	if (from_fs) free(from_fs);
	if (root) free(root);
	if (new_fs && !ok) unref_fs(new_fs);
	return ok;
}

static uint32_t rm_fs_sc(sc_args_t args) {
	char* fs_name = sc_copy_string_x(args.a, args.b);
	if (fs_name == 0) return false;

	proc_rm_fs(current_process(), fs_name);
	free(fs_name);
	return true;
}

//  ---- Spawning new processes & giving them ressources

static uint32_t new_proc_sc(sc_args_t args) {
	process_t *new_proc = new_process(current_process());
	if (new_proc == 0) return 0;

	return new_proc->pid;
}

static uint32_t bind_fs_sc(sc_args_t args) {
	bool ok = false;

	char* old_name = 0;
	char* new_name = 0;
	fs_t* fs = 0;

	process_t *p = process_find_child(current_process(), args.a);
	if (p == 0) goto end_bind_fs;

	new_name = sc_copy_string_x(args.b, args.c);
	if (!new_name) goto end_bind_fs;

	old_name = sc_copy_string_x(args.d, args.e);
	if (!old_name) goto end_bind_fs;

	fs = proc_find_fs(current_process(), old_name);
	if (!fs) goto end_bind_fs;

	ref_fs(fs);
	ok = proc_add_fs(p, fs, new_name);

end_bind_fs:
	if (old_name) free(old_name);
	if (new_name) free(new_name);
	if (fs && !ok) unref_fs(fs);
	return ok;
}

static uint32_t bind_fd_sc(sc_args_t args) {
	bool ok = false;

	fs_handle_t *h = 0;

	process_t *p = process_find_child(current_process(), args.a);
	if (p == 0) goto end_bind_fd;

	h = proc_read_fd(current_process(), args.c);
	if (h == 0) goto end_bind_fd;

	ref_file(h);
	ok = proc_add_fd_as(p, h, args.b);

end_bind_fd:
	if (h && !ok) unref_file(h);
	return ok;
}

static uint32_t proc_exec_sc(sc_args_t args) {
	bool ok = false;

	process_t *p = 0;
	char* exec_name = 0;
	fs_handle_t *h = 0;
	
	p = process_find_child(current_process(), args.a);
	if (p == 0) goto end_exec;

	exec_name = sc_copy_string_x(args.b, args.c);
	if (exec_name == 0) goto end_exec;

	char* sep = strchr(exec_name, ':');
	if (sep == 0) goto end_exec;

	*sep = 0;
	char* file = sep + 1;

	fs_t *fs = proc_find_fs(current_process(), exec_name);
	if (fs == 0) goto end_exec;

	h = fs_open(fs, file, FM_READ | FM_MMAP);
	if (h == 0) h = fs_open(fs, file, FM_READ);
	if (h == 0) goto end_exec;

	proc_entry_t *entry = elf_load(h, p);
	if (entry == 0) goto end_exec;

	ok = start_process(p, entry);

end_exec:
	if (exec_name) free(exec_name);
	if (h) unref_file(h);
	return ok;
}

static uint32_t proc_status_sc(sc_args_t args) {
	proc_status_t *st = (proc_status_t*)args.b;
	probe_for_write(st, sizeof(proc_status_t));

	process_t *p = process_find_child(current_process(), args.a);
	if (p == 0) return false;

	process_get_status(p, st);
	return true;
}

static uint32_t proc_kill_sc(sc_args_t args) {
	proc_status_t *st = (proc_status_t*)args.b;
	probe_for_write(st, sizeof(proc_status_t));

	process_t *p = process_find_child(current_process(), args.a);
	if (p == 0) return false;

	process_exit(p, PS_KILLED, 0);
	process_wait(p, st, true);		// (should return immediately)

	return true;
}

static uint32_t proc_wait_sc(sc_args_t args) {
	proc_status_t *st = (proc_status_t*)args.c;
	probe_for_write(st, sizeof(proc_status_t));

	bool wait = (args.b != 0);

	if (args.a == 0) {
		process_wait_any_child(current_process(), st, wait);
		return true;
	} else {
		process_t *p = process_find_child(current_process(), args.a);
		if (p == 0) return false;

		process_wait(p, st, wait);
		return true;
	}
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

	sc_handlers[SC_MAKE_FS] = make_fs_sc;
	sc_handlers[SC_FS_ADD_SRC] = fs_add_src_sc;
	sc_handlers[SC_SUBFS] = fs_subfs_sc;
	sc_handlers[SC_RM_FS] = rm_fs_sc;

	sc_handlers[SC_NEW_PROC] = new_proc_sc;
	sc_handlers[SC_BIND_FS] = bind_fs_sc;
	sc_handlers[SC_BIND_SUBFS] = fs_subfs_sc; 	// no bind_subfs_sc;
	sc_handlers[SC_BIND_MAKE_FS] = make_fs_sc;	// no bind_make_fs_sc;
	sc_handlers[SC_BIND_FD] = bind_fd_sc;
	sc_handlers[SC_PROC_EXEC] = proc_exec_sc;
	sc_handlers[SC_PROC_STATUS] = proc_status_sc;
	sc_handlers[SC_PROC_KILL] = proc_kill_sc;
	sc_handlers[SC_PROC_WAIT] = proc_wait_sc;
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
			dbg_printf("Unimplemented syscall %d\n", regs->eax);
			regs->eax = -1;
		}
	}
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
