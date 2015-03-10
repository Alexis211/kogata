#pragma once

#include <stdint.h>
#include <stddef.h>

#define TOKEN_LENGTH 16
typedef struct {
	char bytes[TOKEN_LENGTH];
} token_t;

/* vim: set ts=4 sw=4 tw=0 noet :*/
