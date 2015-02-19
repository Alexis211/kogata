#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <syscallproto.h>
#include <mmap.h>

#include <fs.h>
#include <debug.h>

typedef int fd_t;

void dbg_print(const char* str);
void yield();
void exit(int code);

bool mmap(void* addr, size_t size, int mode);
bool mmap_file(fd_t file, size_t offset, void* addr, size_t size, int mode);
bool mchmap(void* addr, int mode);
bool munmap(void* addr);

bool create(const char* name, int type);
bool delete(const char* name);
bool move(const char* oldname, const char* newname);
bool stat(const char* name, stat_t *s);
int ioctl(const char* filename, int command, void* data);

fd_t open(const char* name, int mode);
void close(fd_t file);
size_t read(fd_t file, size_t offset, size_t len, char *buf);
size_t write(fd_t file, size_t offset, size_t len, const char* buf);
bool readdir(fd_t file, dirent_t *d);
bool stat_open(fd_t file, stat_t *s);
int ioctl_open(fd_t file, int command, void* data);
int get_mode(fd_t file);

// more todo

/* vim: set ts=4 sw=4 tw=0 noet :*/
