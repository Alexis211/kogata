#include <kogata/debug.h>
#include <kogata/syscall.h>

#include <kogata/printf.h>

#include <string.h>

static inline uint32_t sc_docall(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t ss, uint32_t dd) {
	uint32_t ret;
	asm volatile("int $0x40"
		:"=a"(ret)
		:"a"(a),"b"(b),"c"(c),"d"(d),"S"(ss),"D"(dd));
	return ret;
}

void sc_dbg_print(const char* str) {
	sc_docall(SC_DBG_PRINT, (uint32_t)str, strlen(str), 0, 0, 0);
}


void sc_yield() {
	sc_docall(SC_YIELD, 0, 0, 0, 0, 0);
}

void sc_exit(int code) {
	sc_docall(SC_EXIT, code, 0, 0, 0, 0);
}

void sc_usleep(int usecs) {
	sc_docall(SC_USLEEP, usecs, 0, 0, 0, 0);
}

bool sc_sys_new_thread(void* eip, void* esp) {
	return sc_docall(SC_NEW_THREAD, (uint32_t)eip, (uint32_t)esp, 0, 0, 0);
}

void sc_exit_thread() {
	sc_docall(SC_EXIT_THREAD, 0, 0, 0, 0, 0);
}

bool sc_mmap(void* addr, size_t size, int mode) {
	return sc_docall(SC_MMAP, (uint32_t)addr, size, mode, 0, 0);
}
bool sc_mmap_file(fd_t file, size_t offset, void* addr, size_t size, int mode) {
	return sc_docall(SC_MMAP_FILE, file, offset, (uint32_t)addr, size, mode);
}
bool sc_mchmap(void* addr, int mode) {
	return sc_docall(SC_MCHMAP, (uint32_t)addr, mode, 0, 0, 0);
}
bool sc_munmap(void* addr) {
	return sc_docall(SC_MUNMAP, (uint32_t)addr, 0, 0, 0, 0);
}

bool sc_create(const char* name, int type) {
	return sc_docall(SC_CREATE, (uint32_t)name, strlen(name), type, 0, 0);
}
bool sc_delete(const char* name) {
	return sc_docall(SC_CREATE, (uint32_t)name, strlen(name), 0, 0, 0);
}
bool sc_move(const char* oldname, const char* newname) {
	return sc_docall(SC_MOVE, (uint32_t)oldname, strlen(oldname), (uint32_t)newname, strlen(newname), 0);
}
bool sc_stat(const char* name, stat_t *s) {
	return sc_docall(SC_STAT, (uint32_t)name, strlen(name), (uint32_t)s, 0, 0);
}

fd_t sc_open(const char* name, int mode) {
	return sc_docall(SC_OPEN, (uint32_t)name, strlen(name), mode, 0, 0);
}
void sc_close(fd_t file) {
	sc_docall(SC_CLOSE, file, 0, 0, 0, 0);
}
size_t sc_read(fd_t file, size_t offset, size_t len, char *buf) {
	return sc_docall(SC_READ, file, offset, len, (uint32_t)buf, 0);
}
size_t sc_write(fd_t file, size_t offset, size_t len, const char* buf) {
	return sc_docall(SC_WRITE, file, offset, len, (uint32_t)buf, 0);
}
bool sc_readdir(fd_t file, size_t ent_no, dirent_t *d) {
	return sc_docall(SC_READDIR, file, ent_no, (uint32_t)d, 0, 0);
}
bool sc_stat_open(fd_t file, stat_t *s) {
	return sc_docall(SC_STAT_OPEN, file, (uint32_t)s, 0, 0, 0);
}
int sc_ioctl(fd_t file, int command, void* data) {
	return sc_docall(SC_IOCTL, file, command, (uint32_t)data, 0, 0);
}
int sc_fctl(fd_t file, int command, void *data) {
	return sc_docall(SC_FCTL, file, command, (uint32_t)data, 0, 0);
}
bool sc_select(sel_fd_t* fds, size_t nfds, int timeout) {
	return sc_docall(SC_SELECT, (uint32_t)fds, nfds, timeout, 0, 0);
}

fd_pair_t sc_make_channel(bool blocking) {
	fd_pair_t ret;
	sc_docall(SC_MK_CHANNEL, blocking, (uint32_t)&ret, 0, 0, 0);
	return ret;
}
fd_t sc_make_shm(size_t s) {
	return sc_docall(SC_MK_SHM, s, 0, 0, 0, 0);
}
bool sc_gen_token(fd_t file, token_t *tok) {
	return sc_docall(SC_GEN_TOKEN, file, (uint32_t)tok, 0, 0, 0);
}
fd_t sc_use_token(token_t *tok) {
	return sc_docall(SC_USE_TOKEN, (uint32_t)tok, 0, 0, 0, 0);
}

bool sc_make_fs(const char* name, const char* driver, fd_t source, const char* options) {
	volatile sc_make_fs_args_t args = {
		.driver = driver,
		.driver_strlen = strlen(driver),
		.fs_name = name,
		.fs_name_strlen = strlen(name),
		.source_fd = source,
		.opts = options,
		.opts_strlen = strlen(options),
		.bind_to_pid = 0,
	};
	return sc_docall(SC_MAKE_FS, (uint32_t)&args, 0, 0, 0, 0);
}
bool sc_fs_add_source(const char* fs, fd_t source, const char* options) {
	return sc_docall(SC_FS_ADD_SRC, (uint32_t)fs, strlen(fs), source, (uint32_t)options, strlen(options));
}
bool sc_fs_subfs(const char* name, const char* orig_fs, const char* root, int ok_modes) {
	volatile sc_subfs_args_t args = {
		.new_name = name,
		.new_name_strlen = strlen(name),
		.from_fs = orig_fs,
		.from_fs_strlen = strlen(orig_fs),
		.root = root,
		.root_strlen = strlen(root),
		.ok_modes = ok_modes,
		.bind_to_pid = 0
	};
	return sc_docall(SC_SUBFS, (uint32_t)(&args), 0, 0, 0, 0);
}
void sc_fs_remove(const char* name) {
	sc_docall(SC_RM_FS, (uint32_t)name, strlen(name), 0, 0, 0);
}

pid_t sc_new_proc() {
	return sc_docall(SC_NEW_PROC, 0, 0, 0, 0, 0);
}
bool sc_bind_fs(pid_t pid, const char* new_name, const char* fs) {
	return sc_docall(SC_BIND_FS, pid, (uint32_t)new_name, strlen(new_name), (uint32_t)fs, strlen(fs));
}
bool sc_bind_subfs(pid_t pid, const char* new_name, const char* orig_fs, const char* root, int ok_modes) {
	sc_subfs_args_t args = {
		.new_name = new_name,
		.new_name_strlen = strlen(new_name),
		.from_fs = orig_fs,
		.from_fs_strlen = strlen(orig_fs),
		.root = root,
		.root_strlen = strlen(root),
		.ok_modes = ok_modes,
		.bind_to_pid = pid
	};
	return sc_docall(SC_BIND_SUBFS, (uint32_t)&args, 0, 0, 0, 0);
}
bool sc_bind_make_fs(pid_t pid, const char* name, const char* driver, fd_t source, const char* options) {
	sc_make_fs_args_t args = {
		.driver = driver,
		.driver_strlen = strlen(driver),
		.fs_name = name,
		.fs_name_strlen = strlen(name),
		.source_fd = source,
		.opts = options,
		.opts_strlen = strlen(options),
		.bind_to_pid = pid,
	};
	return sc_docall(SC_BIND_MAKE_FS, (uint32_t)&args, 0, 0, 0, 0);
}
bool sc_bind_fd(pid_t pid, fd_t new_fd, fd_t fd) {
	return sc_docall(SC_BIND_FD, pid, new_fd, fd, 0, 0);
}
bool sc_proc_exec(pid_t pid, const char* file) {
	return sc_docall(SC_PROC_EXEC, pid, (uint32_t)file, strlen(file), 0, 0);
}
bool sc_proc_status(pid_t pid, proc_status_t *s) {
	return sc_docall(SC_PROC_STATUS, pid, (uint32_t)s, 0, 0, 0);
}
bool sc_proc_kill(pid_t pid, proc_status_t *s) {
	return sc_docall(SC_PROC_KILL, pid, (uint32_t)s, 0, 0, 0);
}
void sc_proc_wait(pid_t pid, bool block, proc_status_t *s) {
	sc_docall(SC_PROC_WAIT, pid, block, (uint32_t)s, 0, 0);
}


/* vim: set ts=4 sw=4 tw=0 noet :*/
