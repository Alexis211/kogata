#pragma once

// Not thread safe

#include <hashtbl.h>

#include <proto/gip.h>
#include <mainloop.h>

typedef struct gip_handler gip_handler_t;

typedef void (*noarg_gip_callback_t)(gip_handler_t *s, gip_msg_header *m);
typedef void (*gip_reply_callback_t)(gip_handler_t *s, gip_msg_header *m, void* msg_data, void* cb_data);

typedef struct {
	noarg_gip_callback_t
		reset, initiate, ok, failure,
		enable_features, disable_features,
		query_mode, set_mode, switch_buffer;
	void (*buffer_info)(gip_handler_t *s, gip_msg_header *m, gip_buffer_info_msg *i);
	void (*mode_info)(gip_handler_t *s, gip_msg_header *m, gip_mode_info_msg *i);
	void (*buffer_damage)(gip_handler_t *s, gip_msg_header *m, gip_buffer_damage_msg *i);
	void (*unknown_msg)(gip_handler_t *s, gip_msg_header *m);
	void (*fd_error)(gip_handler_t *s);
} gip_handler_callbacks_t;

typedef struct gip_handler {
	gip_handler_callbacks_t* cb;
	void* data;

	gip_msg_header			msg_buf;
	gip_buffer_info_msg		buffer_info_msg_buf;
	gip_mode_info_msg		mode_info_msg_buf;
	gip_buffer_damage_msg	buffer_damage_msg_buf;

	hashtbl_t *requests_in_progress;
	uint32_t next_req_id;

	mainloop_fd_t	mainloop_item;
} gip_handler_t;

gip_handler_t *new_gip_handler(gip_handler_callbacks_t *cb, void* data);
void delete_gip_handler(gip_handler_t *h);

// GIP send messages

bool gip_cmd(gip_handler_t *h, gip_msg_header *msg, void* msg_data, gip_reply_callback_t cb, void* cb_data);

bool gip_reply(gip_handler_t *h, gip_msg_header *orig_request, gip_msg_header *msg, void* msg_data);
bool gip_reply_fail(gip_handler_t *h, gip_msg_header *o);
bool gip_reply_ok(gip_handler_t *h, gip_msg_header *o);

bool gip_notify(gip_handler_t *h, gip_msg_header *msg, void* msg_data);

/* vim: set ts=4 sw=4 tw=0 noet :*/
