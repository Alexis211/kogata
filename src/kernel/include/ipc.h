#pragma once

#include <proto/token.h>

#include <vfs.h>

//  ---- Communication channels

#define CHANNEL_BUFFER_SIZE 1000		// 1000 + other channel_t fields = 1024 - epsilon

typedef struct {
	fs_handle_t *a, *b;
} fs_handle_pair_t;

fs_handle_pair_t make_channel(bool blocking);

//  ---- shared memory regions

fs_handle_t* make_shm(size_t size);

//  ---- Tokens for sharing file descriptors between processes

#define TOKEN_LIFETIME 1500000		// in usecs

bool gen_token_for(fs_handle_t *h, token_t *tok);
fs_handle_t *use_token(token_t *tok);

bool token_eq_fun(const void* a, const void* b);
hash_t token_hash_fun(const void* t);

/* vim: set ts=4 sw=4 tw=0 noet :*/
