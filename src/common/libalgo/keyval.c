#include <malloc.h>
#include <string.h>

#include <algo.h>

// Hashing and comparing

hash_t id_hash_fun(const void* v) {
	return (hash_t)v;
}

hash_t str_hash_fun(const void* v) {
	hash_t h = 712;
	for (char* s = (char*)v; *s != 0; s++) {
		h = h * 101 + *s;
	}
	return h;
}

bool id_key_eq_fun(const void* a, const void* b) {
	return a == b;
}

bool str_key_eq_fun(const void* a, const void* b) {
	return strcmp((const char*)a, (const char*)b) == 0;
}

int id_key_cmp_fun(const void* a, const void* b) {
	return (b == a ? 0 : (b > a ? 1 : -1));
}

int str_key_cmp_fun(const void* a, const void* b) {
	return strcmp((const char*)a, (const char*)b);
}

// Freeing functions

void free_key(void* key, void* val) {
	free(key);
}

void free_val(void* key, void* val) {
	free(val);
}

void free_key_val(void* key, void* val) {
	free(key);
	free(val);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
