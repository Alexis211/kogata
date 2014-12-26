#pragma once

#include <stdint.h>
#include <stddef.h>

// The buffer_t type is a simple reference-counted buffer type
// enabling the creation, sharing, slicing and concatenation of buffers
// without having to copy everything each time

// Buffers are always allocated on the core kernel heap (kmalloc/kfree)

// Once a buffer is allocated, its contents is immutable

// Encoding and decoding functions for buffer contents are provided in
// a separate file (encode.h)

struct buffer;
typedef struct buffer buffer_t;

void buffer_ref(buffer_t*);				// increase reference counter
void buffer_unref(buffer_t*);			// decrease reference counter

size_t buffer_len(buffer_t* buf);
size_t read_buffer(buffer_t* buf, char* to, size_t begin, size_t n);	// returns num of bytes read

buffer_t* buffer_from_bytes(const char* data, size_t n);	// bytes are COPIED
buffer_t* buffer_from_bytes_nocopy(const char* data, size_t n, bool own_bytes);	// bytes are NOT COPIED

// these functions GIVE the buffer in order to create the new buffer, ie they do not increase RC
// the buffer is NOT GIVED if the new buffer could not be created (ie retval == 0)
buffer_t* buffer_slice(buffer_t* b, size_t begin, size_t n);
buffer_t* buffer_concat(buffer_t* a, buffer_t* b);

// these functions KEEP a reference on the buffer (ie RC is incremented)
// the RC is NOT INCREMENTED if the new buffer cannot be created
buffer_t* buffer_slice_k(buffer_t* b, size_t begin, size_t n);
buffer_t* buffer_concat_k(buffer_t* a, buffer_t* b);


/* vim: set ts=4 sw=4 tw=0 noet :*/
