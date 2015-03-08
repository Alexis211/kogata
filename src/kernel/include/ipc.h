#pragma once

#include <token.h>
#include <vfs.h>

//  ---- Communication channels

#define CHANNEL_BUFFER_SIZE 1000		// 1000 + other channel_t fields = 1024 - epsilon

typedef struct {
	fs_handle_t *a, *b;
} fs_handle_pair_t;

fs_handle_pair_t make_channel();

//  ---- Tokens for sharing file descriptors between processes

token_t gen_token_for(fs_handle_t *h);
fs_handle_t *use_token(token_t tok);

/* vim: set ts=4 sw=4 tw=0 noet :*/
