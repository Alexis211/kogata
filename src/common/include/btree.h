#pragma once

#include <algo.h>

// A btree may contain several bindings for the same key (in that case they are not ordered)
//	- btree_find returns any item with matching key, or null if none exists
//	- btree_lower returns any item with matching key, or if none returns last item with smaller key
//	- btree_upper returns any item with matching key, or if none returns first item with bigger key
//	- btree_remove removes *all bindings* with matching key
//	- btree_remove_v removes bindings with matching *key and value*
//	- btree_iter_on calls iterator function on all bindings with matching key

// Memory management is same as for hashtbl (a kv_iter_fun_t is called when an item is released)

struct btree;
typedef struct btree btree_t;

btree_t* create_btree(key_cmp_fun_t cf, kv_iter_fun_t on_release);
void delete_btree(btree_t *t);

bool btree_add(btree_t *t, void* key, void* val);
void btree_remove(btree_t *t, const void* key);
void btree_remove_v(btree_t *t, const void* key, const void* value);

void* btree_find(btree_t *i, const void* key);
void* btree_lower(btree_t *i, const void* key, void** actual_key);
void* btree_upper(btree_t *i, const void* key, void** actual_key);
void btree_iter(btree_t *i, kv_iter_fun_t f);
void btree_iter_on(btree_t *i, const void* key, kv_iter_fun_t f);

size_t btree_count(btree_t *i);

/* vim: set ts=4 sw=4 tw=0 noet :*/
