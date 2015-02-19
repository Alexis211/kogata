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
int ioctl(const char* filename, int command, void* data) {
	return call(SC_IOCTL, (uint32_t)filename, strlen(filename), command, (uint32_t)data, 0);
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
bool readdir(fd_t file, dirent_t *d) {
	return call(SC_READDIR, file, (uint32_t)d, 0, 0, 0);
}
bool stat_open(fd_t file, stat_t *s) {
	return call(SC_STAT_OPEN, file, (uint32_t)s, 0, 0, 0);
}
int ioctl_open(fd_t file, int command, void* data) {
	return call(SC_IOCTL_OPEN, file, command, (uint32_t)data, 0, 0);
}
int get_mode(fd_t file) {
	return call(SC_GET_MODE, file, 0, 0, 0, 0);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
