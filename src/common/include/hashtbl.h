#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Simple hashtable structure (key -> void*)
// Supports adding, seeking, removing
// When adding a binding to the table, the previous binding for same key (if exists) is removed

// The hashtbl is allocated with malloc/free
// The keys are not copied in any way by the hashtbl, but there might still be something
// to free, so the create_hashtbl function is given a key freeing function, usually
// null when no freeing is required, or the standard free function.

struct hashtbl;
typedef struct hashtbl hashtbl_t;

typedef size_t hash_t;
typedef hash_t (*hash_fun_t)(const void*);
typedef bool (*key_eq_fun_t)(const void*, const void*);
typedef void (*kv_iter_fun_t)(void* key, void* value);

hashtbl_t* create_hashtbl(key_eq_fun_t ef, hash_fun_t hf, kv_iter_fun_t on_release);	// 0 -> default size
void delete_hashtbl(hashtbl_t* ht);

bool hashtbl_add(hashtbl_t* ht, void* key, void* v);	// true = ok, false on error (OOM for instance)
void* hashtbl_find(hashtbl_t* ht, const void* key);		// null when not found
void hashtbl_remove(hashtbl_t* ht, const void* key);
size_t hashtbl_count(hashtbl_t* ht);
void hashtbl_iter(hashtbl_t* ht, kv_iter_fun_t f);

hash_t id_hash_fun(const void* v);
hash_t str_hash_fun(const void* v);
bool id_key_eq_fun(const void* a, const void* b);
bool str_key_eq_fun(const void* a, const void* b);

void free_key(void* key, void* val);
void free_val(void* key, void* val);
void free_key_val(void* key, void* val);

/* vim: set ts=4 sw=4 tw=0 noet :*/
