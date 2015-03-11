#include <malloc.h>
#include <string.h>

#include <mainloop.h>

mainloop_fd_t *mainloop_fds = 0;
bool mainloop_fds_change = false;
bool mainloop_must_exit = false;

void mainloop_add_fd(mainloop_fd_t* fd) {
	mainloop_fds_change = true;

	fd->next = mainloop_fds;
	mainloop_fds = fd;

	fd->rd_buf_filled = 0;
}

void mainloop_rm_fd(mainloop_fd_t* fd) {
	mainloop_fds_change = true;

	if (mainloop_fds == fd) {
		mainloop_fds = fd->next;
	} else {
		for (mainloop_fd_t *it = mainloop_fds; it->next != 0; it = it->next) {
			if (it->next == fd) {
				it->next = fd->next;
				break;
			}
		}
	}
}

void mainloop_expect(mainloop_fd_t *fd, void* buf, size_t size, buf_full_callback_t cb) {
	fd->rd_buf = buf;
	fd->rd_on_full = cb;
	fd->rd_buf_expect_size = size;
	fd->rd_buf_filled = 0;
}

bool mainloop_nonblocking_write(mainloop_fd_t *fd, void* buf, size_t size, bool must_free_buf) {
	for (int i = 0; i < MAINLOOP_MAX_WR_BUFS; i++) {
		if (fd->wr_bufs[i].buf == 0) {
			fd->wr_bufs[i].buf = buf;
			fd->wr_bufs[i].written = 0;
			fd->wr_bufs[i].size = size;
			fd->wr_bufs[i].must_free = must_free_buf;

			return true;
		}
	}

	return false;
}

void mainloop_run() {
	sel_fd_t *sel_arg = 0;
	int nfds = 0;

	mainloop_fds_change = true;
	mainloop_must_exit = false;
	while(!mainloop_must_exit) {
		if (mainloop_fds_change) {
			nfds = 0;
			for (mainloop_fd_t *fd = mainloop_fds; fd != 0; fd = fd->next)
				nfds++;

			if (sel_arg != 0) free(sel_arg);
			sel_arg = (sel_fd_t*)malloc(nfds * sizeof(sel_fd_t));
			if (sel_arg == 0) {
				dbg_printf("(mainloop) Out of memory.\n");
				return;
			}

			
			mainloop_fds_change = false;
		}

		{	// Setup flags we are waiting for
			int i = 0;
			for (mainloop_fd_t *fd = mainloop_fds; fd != 0; fd = fd->next) {
				sel_arg[i].fd = fd->fd;
				sel_arg[i].req_flags =
					(fd->rd_buf != 0 ? SEL_READ : 0)
					| (fd->wr_bufs[0].buf != 0 ? SEL_WRITE : 0) | SEL_ERROR;
				i++;
			}
		}

		//  ---- Do the select
		bool ok = select(sel_arg, nfds, -1);
		if (!ok) {
			dbg_printf("(mainloop) Failed to select.\n");
			free(sel_arg);
			return;
		}
		
		{	// Parse result
			int i = 0;
			for (mainloop_fd_t *fd = mainloop_fds; fd != 0 && !mainloop_fds_change; fd = fd->next) {
				if (sel_arg[i].got_flags & SEL_ERROR) {
					fd->on_error(fd);
				} else if ((sel_arg[i].got_flags & SEL_READ) && fd->rd_buf != 0) {
					fd->rd_buf_filled +=
						read(fd->fd, 0, fd->rd_buf_expect_size - fd->rd_buf_filled, fd->rd_buf + fd->rd_buf_filled);
					if (fd->rd_buf_filled == fd->rd_buf_expect_size) {
						fd->rd_buf_filled = 0;
						fd->rd_on_full(fd);
					}
				} else if ((sel_arg[i].got_flags & SEL_WRITE) && fd->wr_bufs[0].buf != 0) {
					size_t remain_size = fd->wr_bufs[0].size - fd->wr_bufs[0].written;
					void* write_ptr = fd->wr_bufs[0].buf + fd->wr_bufs[0].written;

					fd->wr_bufs[0].written += write(fd->fd, 0, remain_size, write_ptr);

					if (fd->wr_bufs[0].written == fd->wr_bufs[0].size) {
						if (fd->wr_bufs[0].must_free) free(fd->wr_bufs[0].buf);
						for (int i = 1; i < MAINLOOP_MAX_WR_BUFS; i++) {
							fd->wr_bufs[i-1] = fd->wr_bufs[i];
						}
						memset(&fd->wr_bufs[MAINLOOP_MAX_WR_BUFS-1].buf, 0, sizeof(mainloop_wr_buf_t));
					}
				}
				i++;
			}
		}
	}
}

void mainloop_exit() {
	mainloop_must_exit = true;
}


/* vim: set ts=4 sw=4 tw=0 noet :*/
