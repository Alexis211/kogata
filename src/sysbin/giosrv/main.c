#include <string.h>

#include <malloc.h>

#include <syscall.h>
#include <debug.h>
#include <user_region.h>

#include <proto/keyboard.h>

#include <gip.h>

//  ---- GIP server

typedef struct {
	fb_info_t mode;
	fd_t fd;
} giosrv_t;

void reset(gip_handler_t *s, gip_msg_header *p);
void enable_features(gip_handler_t *s, gip_msg_header *p);
void disable_features(gip_handler_t *s, gip_msg_header *p);
void query_mode(gip_handler_t *s, gip_msg_header *p);
void set_mode(gip_handler_t *s, gip_msg_header *p);
void unknown_msg(gip_handler_t *s, gip_msg_header *p);
void fd_error(gip_handler_t *s);

gip_handler_callbacks_t giosrv_cb = {
	.reset = reset,
	.initiate = 0,
	.ok = 0,
	.failure = 0,
	.enable_features = enable_features,
	.disable_features = disable_features,
	.query_mode = query_mode,
	.set_mode = set_mode,
	.buffer_info = 0,
	.mode_info = 0,
	.buffer_damage = 0,
	.unknown_msg = unknown_msg,
	.fd_error = fd_error,
	.key_down = 0,
	.key_up = 0,
};

giosrv_t srv;
gip_handler_t *gipsrv;

//  ---- KBD listener

typedef struct {
	fd_t fd;
	kbd_event_t ev;
} kbd_listner_t;

kbd_listner_t kbd;

void kbd_handle_event(mainloop_fd_t *fd);
void kbd_on_error(mainloop_fd_t *fd);

int main(int argc, char **argv) {
	dbg_print("[giosrv] Starting up.\n");

	//  ---- Keyboard setup
	kbd.fd = open("io:/input/pckbd", FM_READ);
	if (kbd.fd == 0) PANIC("Could not open keyboard");

	mainloop_fd_t kh;
	memset(&kh, 0, sizeof(kh));

	kh.fd = kbd.fd;
	kh.on_error = kbd_on_error;
	mainloop_expect(&kh, &kbd.ev, sizeof(kbd_event_t), kbd_handle_event);

	mainloop_add_fd(&kh);

	//  ---- VESA setup
	srv.fd = open("io:/display/vesa", FM_IOCTL | FM_READ | FM_WRITE | FM_MMAP);
	if (srv.fd == 0) PANIC("Could not open fbdev");

	int r = ioctl(srv.fd, IOCTL_FB_GET_INFO, &srv.mode);
	ASSERT(r == 1);
	dbg_printf("[giosrv] Running on FB %dx%d\n", srv.mode.width, srv.mode.height);

	//  ---- GIP server setup
	gipsrv = new_gip_handler(&giosrv_cb, &srv);
	ASSERT(gipsrv != 0);

	gipsrv->mainloop_item.fd = 1;
	mainloop_add_fd(&gipsrv->mainloop_item);

	//  ---- Enter main loop
	mainloop_run();

	dbg_printf("[giosrv] Main loop exited, terminating.\n");

	return 0;
}

void send_buffer_info(gip_handler_t *h, giosrv_t *s) {
	gip_msg_header msg = {
		.code = GIPN_BUFFER_INFO,
		.arg = 0,
	};
	gip_buffer_info_msg msg_data;

	msg_data.geom = s->mode;
	if (!gen_token(s->fd, &msg_data.tok)) {
		dbg_printf("[giosrv] Could not generate token for buffer_info_msg.\n");
	} else {
		dbg_printf("[giosrv] Generated token: %x %x\n",
			((uint32_t*)&msg_data.tok)[0], ((uint32_t*)&msg_data.tok)[1]);
		gip_notify(h, &msg, &msg_data);
	}
}

void reset(gip_handler_t *h, gip_msg_header *p) {
	giosrv_t *s = (giosrv_t*)h->data;

	//  ---- Send initiate message
	gip_msg_header msg = {
		.code = GIPR_INITIATE,
		.arg = GIPF_MODESET
	};
	gip_reply(h, p, &msg, 0);

	//  ---- Send buffer information
	send_buffer_info(h, s);
}

void enable_features(gip_handler_t *h, gip_msg_header *p) {
	gip_reply_fail(h, p);
}

void disable_features(gip_handler_t *h, gip_msg_header *p) {
	gip_reply_fail(h, p);
}

void query_mode(gip_handler_t *h, gip_msg_header *p) {
	// TODO
	gip_reply_fail(h, p);
}

void set_mode(gip_handler_t *h, gip_msg_header *p) {
	// TODO
	gip_reply_fail(h, p);
}

void unknown_msg(gip_handler_t *h, gip_msg_header *p) {
	// TODO
	gip_reply_fail(h, p);
}

void fd_error(gip_handler_t *h) {
	// TODO
}

void kbd_handle_event(mainloop_fd_t *fd) {
	gip_msg_header m;
	m.arg = kbd.ev.scancode;
	m.code = (kbd.ev.type == KBD_EVENT_KEYPRESS ? GIPN_KEY_DOWN : GIPN_KEY_UP);

	gip_notify(gipsrv, &m, 0);
}

void kbd_on_error(mainloop_fd_t *fd) {
	// TODO
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
