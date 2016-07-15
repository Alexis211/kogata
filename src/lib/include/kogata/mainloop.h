#pragma once

// These functions are not thread safe, their purpose
// is to multiplex several IO operations on a
// single thread.

#include <kogata/syscall.h>

#define MAINLOOP_MAX_WR_BUFS 4

typedef struct mainloop_fd mainloop_fd_t;

typedef void (*buf_full_callback_t)(mainloop_fd_t *fd);
typedef void (*fd_error_callback_t)(mainloop_fd_t *fd);

typedef struct {
	size_t size, written;
	void* buf;
	bool must_free;
} mainloop_wr_buf_t;

typedef struct mainloop_fd {
	fd_t fd;
	
	size_t rd_buf_expect_size, rd_buf_filled;
	void* rd_buf;

	mainloop_wr_buf_t wr_bufs[MAINLOOP_MAX_WR_BUFS];

	void* data;

	buf_full_callback_t rd_on_full;
	fd_error_callback_t on_error;

	mainloop_fd_t *next;
} mainloop_fd_t;

void mainloop_add_fd(mainloop_fd_t* fd);
void mainloop_rm_fd(mainloop_fd_t* fd);

void mainloop_expect(mainloop_fd_t *fd, void* buf, size_t size, buf_full_callback_t cb);
bool mainloop_nonblocking_write(mainloop_fd_t *fd, void* buf, size_t size, bool must_free_buf);

void mainloop_run();
void mainloop_exit();

/* vim: set ts=4 sw=4 tw=0 noet :*/
