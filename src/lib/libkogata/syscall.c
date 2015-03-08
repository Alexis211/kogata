#include <debug.h>
#include <syscall.h>

#include <string.h>
#include <printf.h>

static inline uint32_t call(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t ss, uint32_t dd) {
	uint32_t ret;
	asm volatile("int $0x40"
		:"=a"(ret)
		:"a"(a),"b"(b),"c"(c),"d"(d),"S"(ss),"D"(dd));
	return ret;
}

void dbg_print(const char* str) {
	call(SC_DBG_PRINT, (uint32_t)str, strlen(str), 0, 0, 0);
}

void dbg_printf(const char* fmt, ...) {
	va_list ap;
	char buffer[256];

	va_start(ap, fmt);
	vsnprintf(buffer, 256, fmt, ap);
	va_end(ap);

	dbg_print(buffer);
}

void yield() {
	call(SC_YIELD, 0, 0, 0, 0, 0);
}

void exit(int code) {
	call(SC_EXIT, code, 0, 0, 0, 0);
}

void usleep(int usecs) {
	call(SC_USLEEP, usecs, 0, 0, 0, 0);
}

bool new_thread(entry_t entry, void* data) {
	// TODO
	return false;
}

void exit_thread() {
	call(SC_EXIT_THREAD, 0, 0, 0, 0, 0);
}

bool mmap(void* addr, size_t size, int mode) {
	return call(SC_MMAP, (uint32_t)addr, size, mode, 0, 0);
}
bool mmap_file(fd_t file, size_t offset, void* addr, size_t size, int mode) {
	return call(SC_MMAP_FILE, file, offset, (uint32_t)addr, size, mode);
}
bool mchmap(void* addr, int mode) {
	return call(SC_MCHMAP, (uint32_t)addr, mode, 0, 0, 0);
}
bool munmap(void* addr) {
	return call(SC_MUNMAP, (uint32_t)addr, 0, 0, 0, 0);
}

bool create(const char* name, int type) {
	return call(SC_CREATE, (uint32_t)name, strlen(name), type, 0, 0);
}
bool delete(const char* name) {
	return call(SC_CREATE, (uint32_t)name, strlen(name), 0, 0, 0);
}
bool move(const char* oldname, const char* newname) {
	return call(SC_MOVE, (uint32_t)oldname, strlen(oldname), (uint32_t)newname, strlen(newname), 0);
}
bool stat(const char* name, stat_t *s) {
	return call(SC_STAT, (uint32_t)name, strlen(name), (uint32_t)s, 0, 0);
}

fd_t open(const char* name, int mode) {
	return call(SC_OPEN, (uint32_t)name, strlen(name), mode, 0, 0);
}
void close(fd_t file) {
	call(SC_CLOSE, file, 0, 0, 0, 0);
}
size_t read(fd_t file, size_t offset, size_t len, char *buf) {
	return call(SC_READ, file, offset, len, (uint32_t)buf, 0);
}
size_t write(fd_t file, size_t offset, size_t len, const char* buf) {
	return call(SC_WRITE, file, offset, len, (uint32_t)buf, 0);
}
bool readdir(fd_t file, size_t ent_no, dirent_t *d) {
	return call(SC_READDIR, file, ent_no, (uint32_t)d, 0, 0);
}
bool stat_open(fd_t file, stat_t *s) {
	return call(SC_STAT_OPEN, file, (uint32_t)s, 0, 0, 0);
}
int ioctl(fd_t file, int command, void* data) {
	return call(SC_IOCTL, file, command, (uint32_t)data, 0, 0);
}
int get_mode(fd_t file) {
	return call(SC_GET_MODE, file, 0, 0, 0, 0);
}

fd_pair_t make_channel(bool blocking) {
	fd_pair_t ret;
	call(SC_MK_CHANNEL, blocking, (uint32_t)&ret, 0, 0, 0);
	return ret;
}
bool gen_token(fd_t file, token_t *tok) {
	return call(SC_GEN_TOKEN, file, (uint32_t)tok, 0, 0, 0);
}
fd_t use_token(token_t *tok) {
	return call(SC_USE_TOKEN, (uint32_t)tok, 0, 0, 0, 0);
}

bool make_fs(const char* name, const char* driver, fd_t source, const char* options) {
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
	return call(SC_MAKE_FS, (uint32_t)&args, 0, 0, 0, 0);
}
bool fs_add_source(const char* fs, fd_t source, const char* options) {
	return call(SC_FS_ADD_SRC, (uint32_t)fs, strlen(fs), source, (uint32_t)options, strlen(options));
}
bool fs_subfs(const char* name, const char* orig_fs, const char* root, int ok_modes) {
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
	return call(SC_SUBFS, (uint32_t)(&args), 0, 0, 0, 0);
}
void fs_remove(const char* name) {
	call(SC_RM_FS, (uint32_t)name, strlen(name), 0, 0, 0);
}

pid_t new_proc() {
	return call(SC_NEW_PROC, 0, 0, 0, 0, 0);
}
bool bind_fs(pid_t pid, const char* new_name, const char* fs) {
	return call(SC_BIND_FS, pid, (uint32_t)new_name, strlen(new_name), (uint32_t)fs, strlen(fs));
}
bool bind_subfs(pid_t pid, const char* new_name, const char* orig_fs, const char* root, int ok_modes) {
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
	return call(SC_BIND_SUBFS, (uint32_t)&args, 0, 0, 0, 0);
}
bool bind_make_fs(pid_t pid, const char* name, const char* driver, fd_t source, const char* options) {
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
	return call(SC_BIND_MAKE_FS, (uint32_t)&args, 0, 0, 0, 0);
}
bool bind_fd(pid_t pid, fd_t new_fd, fd_t fd) {
	return call(SC_BIND_FD, new_fd, fd, 0, 0, 0);
}
bool proc_exec(pid_t pid, const char* file) {
	return call(SC_PROC_EXEC, pid, (uint32_t)file, strlen(file), 0, 0);
}
bool proc_status(pid_t pid, proc_status_t *s) {
	return call(SC_PROC_STATUS, pid, (uint32_t)s, 0, 0, 0);
}
bool proc_kill(pid_t pid, proc_status_t *s) {
	return call(SC_PROC_KILL, pid, (uint32_t)s, 0, 0, 0);
}
void proc_wait(pid_t pid, bool block, proc_status_t *s) {
	call(SC_PROC_WAIT, pid, block, (uint32_t)s, 0, 0);
}




/* vim: set ts=4 sw=4 tw=0 noet :*/
