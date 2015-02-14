#pragma once

#include <algo.h>

// Simple hashtable structure (key -> void*)
// Supports adding, seeking, removing, iterating
// When adding a binding to the table, the previous binding for same key (if exists) is removed

// The hashtbl is allocated with malloc/free
// The keys/values are not copied by the hastbl, but when a key/value pair is removed from the
// table some operations may be required (freeing memory), so a kv_iter_fun_t is passed when the
// table is created which will be called whenever a k/v is released (on hashtbl_remove and delete_hashtbl)

// A hashtbl may have only one binding for a given key (on add, previous binding is removed if necessary)

struct hashtbl;
typedef struct hashtbl hashtbl_t;

hashtbl_t* create_hashtbl(key_eq_fun_t ef, hash_fun_t hf, kv_iter_fun_t on_release);
void delete_hashtbl(hashtbl_t* ht);

bool hashtbl_add(hashtbl_t* ht, void* key, void* v);	// true = ok, false on error (OOM for instance)
void hashtbl_remove(hashtbl_t* ht, const void* key);

void* hashtbl_find(hashtbl_t* ht, const void* key);		// null when not found
void hashtbl_iter(hashtbl_t* ht, kv_iter_fun_t f);

size_t hashtbl_count(hashtbl_t* ht);

/* vim: set ts=4 sw=4 tw=0 noet :*/
