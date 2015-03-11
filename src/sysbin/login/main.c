#include <string.h>
#include <malloc.h>

#include <debug.h>

#include <gip.h>

typedef struct {
	framebuffer_info_t mode;
	void* map;
	fd_t fd;
	uint32_t features;
} loginc_t;

void c_buffer_info(gip_handler_t *s, gip_msg_header *p, gip_buffer_info_msg *m);
void c_unknown_msg(gip_handler_t *s, gip_msg_header *p);
void c_fd_error(gip_handler_t *s);

void c_async_initiate(gip_handler_t *s, gip_msg_header *p, void* data);

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
	// TODO
}

void c_unknown_msg(gip_handler_t *s, gip_msg_header *p) {
	// TODO
}

void c_fd_error(gip_handler_t *s) {
	// TODO
}

void c_async_initiate(gip_handler_t *s, gip_msg_header *p, void* data) {
	dbg_printf("[login] Got initiate reply to reset request.\n");
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
