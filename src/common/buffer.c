#include <malloc.h>

#include <buffer.h>
#include <string.h>

#include <debug.h>

// three types of buffers
#define T_BYTES		1
#define T_SLICE		2
#define T_CONCAT	3

struct buffer {
	uint16_t rc, type;		// takes less space like this
	size_t len;
	union {
		struct {
			const char* data;
			bool owned;
		} bytes;
		struct {
			struct buffer *buf;
			size_t begin;
		} slice;
		struct {
			struct buffer *a, *b;
		} concat;
	};
};

void buffer_ref(buffer_t *b) {
	b->rc++;
}

void buffer_unref(buffer_t *b) {
	b->rc--;
	if (b->rc == 0) {
		switch (b->type) {
			case T_BYTES:
				if (b->bytes.owned) free((void*)b->bytes.data);
				break;
			case T_SLICE:
				buffer_unref(b->slice.buf);
				break;
			case T_CONCAT:
				buffer_unref(b->concat.a);
				buffer_unref(b->concat.b);
				break;
			default:
				ASSERT(false);
		}
		free(b);
	}
}

size_t buffer_size(buffer_t *b) {
	return b->len;
}

size_t read_buffer(buffer_t *b, char* dest, size_t begin, size_t n) {
	if (begin >= b->len) return 0;
	if (begin + n >= b->len) n = b->len - begin;

	switch (b->type) {
		case T_BYTES:
			memcpy(dest, b->bytes.data + begin, n);
			break;
		case T_SLICE:
			read_buffer(b->slice.buf, dest, begin + b->slice.begin, n);
			break;
		case T_CONCAT: {
			size_t la = b->concat.a->len;
			if (begin < la) {
				size_t r = read_buffer(b->concat.a, dest, begin, n);
				if (r < n) {
					ASSERT(read_buffer(b->concat.b, dest, 0, n - r) == n - r);
				}
			} else {
				ASSERT(read_buffer(b->concat.b, dest, begin - la, n) == n);
			}
			break;
		}
		default:
			ASSERT(false);
	}

	return n;
}

// ========================= //
// BUFFER CREATION FUNCTIONS //
// ========================= //

buffer_t *buffer_from_bytes_nocopy(const char* data, size_t n, bool own_bytes) {
	buffer_t *b = (buffer_t*)malloc(sizeof(buffer_t));
	if (b == 0) return 0;

	b->rc = 1;
	b->type = T_BYTES;
	b->len = n;
	b->bytes.data = data;
	b->bytes.owned = own_bytes;

	return b;
}
buffer_t *buffer_from_bytes(const char* data, size_t n) {
	char* data2 = (char*)malloc(n);
	if (data2 == 0) return 0;

	memcpy(data2, data, n);

	buffer_t *b = buffer_from_bytes_nocopy(data2, n, true);
	if (b == 0) {
		free(data2);
		return 0;
	}

	return b;
}


buffer_t* buffer_slice(buffer_t* src, size_t begin, size_t n) {
	if (begin + n > src->len) return 0;	// invalid request

	buffer_t *b = (buffer_t*)malloc(sizeof(buffer_t));
	if (b == 0) return 0;

	b->rc = 1;
	b->type = T_SLICE;
	b->len = n;
	b->slice.buf = src;
	b->slice.begin = begin;

	return b;
}

buffer_t* buffer_concat(buffer_t* a, buffer_t* b) {
	buffer_t *r = (buffer_t*)malloc(sizeof(buffer_t));
	if (r == 0) return r;

	r->rc = 1;
	r->type = T_CONCAT;
	r->len = a->len + b->len;
	r->concat.a = a;
	r->concat.b = b;

	return r;
}

buffer_t* buffer_slice_k(buffer_t *b, size_t begin, size_t n) {
	buffer_t *r = buffer_slice(b, begin, n);
	if (r != 0) {
		buffer_ref(b);
	}
	return r;
}

buffer_t* buffer_concat_k(buffer_t *a, buffer_t *b) {
	buffer_t *r = buffer_concat(a, b);
	if (r != 0) {
		buffer_ref(a);
		buffer_ref(b);
	}
	return r;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
