#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MIN(a, b)   ((a)<(b)?(a):(b))
#define MAX(a, b)   ((a)>(b)?(a):(b))
#define ABS(a)		((a)<0?-(a):(a))

// ============================================================= //
// FUNCTION TYPES FOR KEY-VALUE DATA STRUCTURES (HASHTBL, BTREE) //

typedef uint32_t hash_t;
typedef hash_t (*hash_fun_t)(const void*);

typedef int (*key_cmp_fun_t)(const void*, const void*);
typedef bool (*key_eq_fun_t)(const void*, const void*);

typedef void (*kv_iter_fun_t)(void* key, void* value);

// void* is considered as an unsigned integer (and not a pointer)
hash_t id_hash_fun(const void* v);
bool id_key_eq_fun(const void* a, const void* b);
int id_key_cmp_fun(const void* a, const void* b);

// void* considered as char*
hash_t str_hash_fun(const void* v);
bool str_key_eq_fun(const void* a, const void* b);
int str_key_cmp_fun(const void* a, const void* b);

// Freeing functions (they are of type kv_iter_fun_t)
void free_key(void* key, void* val);
void free_val(void* key, void* val);
void free_key_val(void* key, void* val);

/* vim: set ts=4 sw=4 tw=0 noet :*/
