#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <syscallproto.h>
#include <mmap.h>

#include <fs.h>
#include <debug.h>

typedef int fd_t;
typedef int pid_t;

void dbg_print(const char* str);
void yield();
void exit(int code);
void usleep(int usecs);

bool mmap(void* addr, size_t size, int mode);
bool mmap_file(fd_t file, size_t offset, void* addr, size_t size, int mode);
bool mchmap(void* addr, int mode);
bool munmap(void* addr);

bool create(const char* name, int type);
bool delete(const char* name);
bool move(const char* oldname, const char* newname);
bool stat(const char* name, stat_t *s);

fd_t open(const char* name, int mode);
void close(fd_t file);
size_t read(fd_t file, size_t offset, size_t len, char *buf);
size_t write(fd_t file, size_t offset, size_t len, const char* buf);
bool readdir(fd_t file, size_t ent_no, dirent_t *d);
bool stat_open(fd_t file, stat_t *s);
int ioctl(fd_t file, int command, void* data);
int get_mode(fd_t file);

bool make_fs(const char* name, const char* driver, fd_t source, const char* options);
bool fs_add_source(const char* fs, fd_t source, const char* options);
bool fs_subfs(const char* name, const char* orig_fs, const char* root, int ok_modes);
void fs_remove(const char* name);

pid_t new_proc();
bool bind_fs(pid_t pid, const char* new_name, const char* fs);
bool bind_subfs(pid_t pid, const char* new_name, const char* fs, const char* root, int ok_modes);
bool bind_fd(pid_t pid, fd_t new_fd, fd_t fd);
bool proc_exec(pid_t pid, const char* file);
bool proc_status(pid_t pid, proc_status_t *s);
bool proc_kill(pid_t pid, proc_status_t *s);
void proc_wait(pid_t pid, bool wait, proc_status_t *s);

/* vim: set ts=4 sw=4 tw=0 noet :*/
