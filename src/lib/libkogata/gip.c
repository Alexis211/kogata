#include <string.h>
#include <malloc.h>

#include <gip.h>

typedef struct {
	gip_reply_callback_t cb;
	void* data;
} gip_cmd_t;

void giph_msg_header(mainloop_fd_t *fd);
void giph_buffer_info(mainloop_fd_t *fd);
void giph_mode_info(mainloop_fd_t *fd);
void giph_buffer_damage(mainloop_fd_t *fd);

void gip_error(mainloop_fd_t *fd);

gip_handler_t *new_gip_handler(gip_handler_callbacks_t *cb, void* data) {
	gip_handler_t *h = (gip_handler_t*)malloc(sizeof(gip_handler_t));
	if (h == 0) return 0;

	memset(h, 0, sizeof(gip_handler_t));

	h->cb = cb;
	h->data = data;

	h->requests_in_progress = create_hashtbl(id_key_eq_fun, id_hash_fun, free_val);
	if (h->requests_in_progress == 0) {
		free(h);
		return 0;
	}

	h->mainloop_item.data = h;
	h->mainloop_item.on_error = &gip_error;
	h->next_req_id = 1;

	mainloop_expect(&h->mainloop_item, &h->msg_buf, sizeof(gip_msg_header), giph_msg_header);

	return h;
}

void delete_gip_handler(gip_handler_t *h) {
	delete_hashtbl(h->requests_in_progress);
	free(h);
}

bool gip_send_msg(gip_handler_t *h, gip_msg_header *msg, void* msg_data) {
	//  ---- Write message
	size_t extra_size = 0;
	if (msg->code == GIPN_BUFFER_INFO) extra_size = sizeof(gip_buffer_info_msg);
	if (msg->code == GIPR_MODE_INFO) extra_size = sizeof(gip_mode_info_msg);
	if (msg->code == GIPN_BUFFER_DAMAGE) extra_size = sizeof(gip_buffer_damage_msg);

	bool ok = false;

	char* buf = (char*)malloc(sizeof(gip_msg_header) + extra_size);

	if (buf != 0) {
		memcpy(buf, msg, sizeof(gip_msg_header));
		if (extra_size) memcpy(buf + sizeof(gip_msg_header), msg_data, extra_size);
		ok = mainloop_nonblocking_write(&h->mainloop_item, buf, sizeof(gip_msg_header) + extra_size, true);
	}

	if (!ok) {
		dbg_printf("GIP warning: failed to send message (type %d)\n", msg->code);
		if (buf) free(buf);
	}

	return ok;
}

bool gip_cmd(gip_handler_t *h, gip_msg_header *msg, void* msg_data, gip_reply_callback_t cb, void* cb_data) {
	msg->req_id = 0;

	//  ---- Add callback handler
	if (cb != 0) {
		gip_cmd_t *c = (gip_cmd_t*)malloc(sizeof(gip_cmd_t));
		if (c == 0) return false;

		c->cb = cb;
		c->data = cb_data;

		while(msg->req_id == 0) msg->req_id = (h->next_req_id++);

		bool add_ok = hashtbl_add(h->requests_in_progress, (void*)msg->req_id, c);
		if (!add_ok) {
			free(c);
			return false;
		}
	}

	bool ok = gip_send_msg(h, msg, msg_data);
	if (!ok && msg->req_id != 0) hashtbl_remove(h->requests_in_progress, (void*)msg->req_id);

	return ok;
}

bool gip_reply(gip_handler_t *h, gip_msg_header *orig_request, gip_msg_header *msg, void* msg_data) {
	msg->req_id = orig_request->req_id;
	return gip_send_msg(h, msg, msg_data);
}

bool gip_reply_fail(gip_handler_t *h, gip_msg_header *o) {
	gip_msg_header m = {
		.code = GIPR_FAILURE,
		.arg = 0,
	};
	return gip_reply(h, o, &m, 0);
}

bool gip_reply_ok(gip_handler_t *h, gip_msg_header *o) {
	gip_msg_header m = {
		.code = GIPR_OK,
		.arg = 0,
	};
	return gip_reply(h, o, &m, 0);
}

bool gip_notify(gip_handler_t *h, gip_msg_header *msg, void* msg_data) {
	msg->req_id = 0;
	return gip_send_msg(h, msg, msg_data);
}

//  ---- Message handlers

void giph_got_reply(gip_handler_t *h, gip_msg_header *msg, void* msg_data) {
	gip_cmd_t *c = (gip_cmd_t*)hashtbl_find(h->requests_in_progress, (void*)msg->req_id);;
	if (c != 0) {
		ASSERT(c->cb != 0);
		c->cb(h, msg, msg_data, c->data);
		hashtbl_remove(h->requests_in_progress, (void*)msg->req_id);
	}
}

void giph_msg_header(mainloop_fd_t *fd) {
	gip_handler_t *h = (gip_handler_t*)fd->data;
	ASSERT(fd == &h->mainloop_item);

	int code = h->msg_buf.code;
	/*dbg_printf("Got GIP header, code %d\n", code);*/

	noarg_gip_callback_t use_cb = 0;
	if (code ==	GIPC_RESET) {
		use_cb = h->cb->reset;
	} else if (code == GIPR_INITIATE) {
		use_cb = h->cb->initiate;
		giph_got_reply(h, &h->msg_buf, 0);
	} else if (code == GIPR_OK) {
		use_cb = h->cb->ok;
		giph_got_reply(h, &h->msg_buf, 0);
	} else if (code == GIPR_FAILURE) {
		use_cb = h->cb->failure;
		giph_got_reply(h, &h->msg_buf, 0);
	} else if (code == GIPC_ENABLE_FEATURES) {
		use_cb = h->cb->enable_features;
	} else if (code == GIPC_DISABLE_FEATURES) {
		use_cb = h->cb->disable_features;
	} else if (code == GIPC_QUERY_MODE) {
		use_cb = h->cb->query_mode;
	} else if (code == GIPC_SET_MODE) {
		use_cb = h->cb->set_mode;
	} else if (code == GIPC_SWITCH_BUFFER) {
		use_cb = h->cb->switch_buffer;
	} else if (code == GIPN_BUFFER_INFO) {
		mainloop_expect(fd, &h->buffer_info_msg_buf, sizeof(gip_buffer_info_msg), giph_buffer_info);
	} else if (code == GIPR_MODE_INFO) {
		mainloop_expect(fd, &h->mode_info_msg_buf, sizeof(gip_mode_info_msg), giph_mode_info);
	} else if (code == GIPN_BUFFER_DAMAGE) {
		mainloop_expect(fd, &h->buffer_damage_msg_buf, sizeof(gip_buffer_damage_msg), giph_buffer_damage);
	} else if (code == GIPN_KEY_DOWN) {
		use_cb = h->cb->key_down;
	} else if (code == GIPN_KEY_UP) {
		use_cb = h->cb->key_up;
	} else {
		use_cb = h->cb->unknown_msg;
	}

	if (use_cb) use_cb(h, &h->msg_buf);
}

void giph_buffer_info(mainloop_fd_t *fd) {
	gip_handler_t *h = (gip_handler_t*)fd->data;

	if (h->cb->buffer_info) h->cb->buffer_info(h, &h->msg_buf, &h->buffer_info_msg_buf);

	mainloop_expect(&h->mainloop_item, &h->msg_buf, sizeof(gip_msg_header), giph_msg_header);
}

void giph_mode_info(mainloop_fd_t *fd) {
	gip_handler_t *h = (gip_handler_t*)fd->data;

	if (h->cb->mode_info) h->cb->mode_info(h, &h->msg_buf, &h->mode_info_msg_buf);
	giph_got_reply(h, &h->msg_buf, &h->mode_info_msg_buf);

	mainloop_expect(&h->mainloop_item, &h->msg_buf, sizeof(gip_msg_header), giph_msg_header);
}

void giph_buffer_damage(mainloop_fd_t *fd) {
	gip_handler_t *h = (gip_handler_t*)fd->data;

	if (h->cb->buffer_damage) h->cb->buffer_damage(h, &h->msg_buf, &h->buffer_damage_msg_buf);

	mainloop_expect(&h->mainloop_item, &h->msg_buf, sizeof(gip_msg_header), giph_msg_header);
}

void gip_error(mainloop_fd_t *fd) {
	gip_handler_t *h = (gip_handler_t*)fd->data;

	if (h->cb->fd_error) h->cb->fd_error(h);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
