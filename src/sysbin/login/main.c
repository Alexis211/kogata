#include <string.h>
#include <malloc.h>
#include <user_region.h>
#include <debug.h>

#include <gip.h>

typedef struct {
	framebuffer_info_t mode;

	size_t fb_size;
	void* map;

	fd_t fd;

	uint32_t sv_features, cl_features;
} loginc_t;

void c_buffer_info(gip_handler_t *s, gip_msg_header *p, gip_buffer_info_msg *m);
void c_unknown_msg(gip_handler_t *s, gip_msg_header *p);
void c_fd_error(gip_handler_t *s);

void c_async_initiate(gip_handler_t *s, gip_msg_header *p, void* msgdata, void* hdata);

gip_handler_callbacks_t loginc_cb = {
	.reset = 0,
	.initiate = 0,
	.ok = 0,
	.failure = 0,
	.enable_features = 0,
	.disable_features = 0,
	.query_mode = 0,
	.set_mode = 0,
	.buffer_info = c_buffer_info,
	.mode_info = 0,
	.buffer_damage = 0,
	.unknown_msg = c_unknown_msg,
	.fd_error = c_fd_error,
};

int main(int argc, char **argv) {
	dbg_print("[login] Starting up.\n");

	loginc_t loginc;
	memset(&loginc, 0, sizeof(loginc));

	gip_handler_t *h = new_gip_handler(&loginc_cb, &loginc);
	ASSERT(h != 0);

	h->mainloop_item.fd = 1;
	mainloop_add_fd(&h->mainloop_item);

	gip_msg_header reset_msg = { .code = GIPC_RESET, .arg = 0 };
	gip_cmd(h, &reset_msg, 0, c_async_initiate, 0);

	mainloop_run();

	return 0;
}

void c_buffer_info(gip_handler_t *s, gip_msg_header *p, gip_buffer_info_msg *m) {
	dbg_printf("[login] Got buffer info msg\n");

	loginc_t *c = (loginc_t*)s->data;

	if (c->fd != 0) close(c->fd);
	if (c->map != 0) {
		munmap(c->map);
		region_free(c->map);
	}

	c->fd = use_token(&m->tok);
	if (c->fd != 0) {
		memcpy(&c->mode, &m->geom, sizeof(framebuffer_info_t));

		dbg_printf("[login] Got buffer on FD %d, %dx%dx%d\n",
			c->fd, c->mode.width, c->mode.height, c->mode.bpp);

		c->fb_size = c->mode.pitch * c->mode.height;

		c->map = region_alloc(c->fb_size, "Framebuffer");
		if (c->map != 0) {
			bool ok = mmap_file(c->fd, 0, c->map, c->fb_size, MM_READ | MM_WRITE);
			if (ok) {
				memset(c->map, 0, c->fb_size);
			} else {
				dbg_printf("[login] Could not mmap buffer.\n");
				region_free(c->map);
				c->map = 0;
			}
		}
	}
}

void c_unknown_msg(gip_handler_t *s, gip_msg_header *p) {
	// TODO
}

void c_fd_error(gip_handler_t *s) {
	// TODO
}

void c_async_initiate(gip_handler_t *s, gip_msg_header *p, void* msgdata, void* hdata) {
	loginc_t *c = (loginc_t*)s->data;
	c->sv_features = p->arg;

	dbg_printf("[login] Got initiate reply to reset request (features 0x%p).\n", c->sv_features);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
