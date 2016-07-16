#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <proto/syscall.h>
#include <proto/mmap.h>
#include <proto/fs.h>
#include <proto/token.h>

#include <kogata/debug.h>

typedef void (*entry_t)(void*);

void sc_dbg_print(const char* str);
void sc_yield();
void sc_exit(int code);
void sc_usleep(int usecs);
bool sc_sys_new_thread(void* eip, void* esp);
void sc_exit_thread();

bool sc_mmap(void* addr, size_t size, int mode);
bool sc_mmap_file(fd_t file, size_t offset, void* addr, size_t size, int mode);
bool sc_mchmap(void* addr, int mode);
bool sc_munmap(void* addr);

bool sc_create(const char* name, int type);
bool sc_delete(const char* name);
bool sc_move(const char* oldname, const char* newname);
bool sc_stat(const char* name, stat_t *s);

fd_t sc_open(const char* name, int mode);
void sc_close(fd_t file);
size_t sc_read(fd_t file, size_t offset, size_t len, char *buf);
size_t sc_write(fd_t file, size_t offset, size_t len, const char* buf);
bool sc_readdir(fd_t file, size_t ent_no, dirent_t *d);
bool sc_stat_open(fd_t file, stat_t *s);
int sc_ioctl(fd_t file, int command, void* data);
int sc_fctl(fd_t file, int command, void* data);
bool sc_select(sel_fd_t* fds, size_t nfds, int timeout);

fd_pair_t sc_make_channel(bool blocking);
fd_t sc_make_shm(size_t size);
bool sc_gen_token(fd_t file, token_t *tok);
fd_t sc_use_token(token_t *tok);

bool sc_make_fs(const char* name, const char* driver, fd_t source, const char* options);
bool sc_fs_add_source(const char* fs, fd_t source, const char* options);
bool sc_fs_subfs(const char* name, const char* orig_fs, const char* root, int ok_modes);
void sc_fs_remove(const char* name);

pid_t sc_new_proc();
bool sc_bind_fs(pid_t pid, const char* new_name, const char* fs);
bool sc_bind_subfs(pid_t pid, const char* new_name, const char* fs, const char* root, int ok_modes);
bool sc_bind_make_fs(pid_t pid, const char* name, const char* driver, fd_t source, const char* options);
bool sc_bind_fd(pid_t pid, fd_t new_fd, fd_t fd);
bool sc_proc_exec(pid_t pid, const char* file);
bool sc_proc_status(pid_t pid, proc_status_t *s);
bool sc_proc_kill(pid_t pid, proc_status_t *s);
void sc_proc_wait(pid_t pid, bool wait, proc_status_t *s);

/* vim: set ts=4 sw=4 tw=0 noet :*/
