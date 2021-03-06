#pragma once

#include <stdint.h>
#include <stddef.h>

// Header is in common/, but implementation is not.

void* malloc(size_t sz);
void free(void* ptr);
void* calloc(size_t nmemb, size_t sz);
void* realloc(void* ptr, size_t sz);

/* vim: set ts=4 sw=4 tw=0 noet :*/
