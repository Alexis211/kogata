#include <hashtbl.h>
#include <malloc.h>
#include <string.h>

#define DEFAULT_INIT_SIZE 16
#define SLOT_OF_HASH(k, nslots) (((size_t)(k)*21179)%(size_t)(nslots))

typedef struct hashtbl_item {
	void* key;
	void* val;
	struct hashtbl_item *next;
} hashtbl_item_t;

// When nitems > size * 3/4, size is multiplied by two
// When nitems < size * 1/4, size is divided by two
struct hashtbl {
	key_eq_fun_t ef;
	hash_fun_t hf;
	key_free_fun_t kff;
	size_t size, nitems;
	hashtbl_item_t **items;
};

hashtbl_t *create_hashtbl(key_eq_fun_t ef, hash_fun_t hf, key_free_fun_t kff, size_t initial_size) {
	hashtbl_t *ht = (hashtbl_t*)malloc(sizeof(hashtbl_t));
	if (ht == 0) return 0;

	ht->ef = ef;
	ht->hf = hf;
	ht->kff = kff;

	ht->size = (initial_size == 0 ? DEFAULT_INIT_SIZE : initial_size);
	ht->nitems = 0;

	ht->items = (hashtbl_item_t**)malloc(ht->size * sizeof(hashtbl_item_t*));
	if (ht->items == 0) {
		free(ht);
		return 0;
	}

	for (size_t i = 0; i < ht->size; i++) ht->items[i] = 0;

	return ht;
}

void delete_hashtbl(hashtbl_t *ht) {
	// Free items
	for (size_t i = 0; i < ht->size; i++) {
		while (ht->items[i] != 0) {
			hashtbl_item_t *n = ht->items[i]->next;
			if (ht->kff) ht->kff(ht->items[i]->key);
			free(ht->items[i]);
			ht->items[i] = n;
		}
	}

	// Free table
	free(ht->items);
	free(ht);
}

static void hashtbl_check_size(hashtbl_t *ht) {
	size_t nsize = 0;
	if (4 * ht->nitems < ht->size) nsize = ht->size / 2;
	if (4 * ht->nitems > 3 * ht->size) nsize = ht->size * 2;

	if (nsize != 0) {
		hashtbl_item_t **nitems = (hashtbl_item_t**)malloc(nsize * sizeof(hashtbl_item_t*));
		if (nitems == 0) return;	// if we can't realloc, too bad, we just lose space

		for (size_t i = 0; i < nsize; i++) nitems[i] = 0;

		// rehash
		for (size_t i = 0; i < ht->size; i++) {
			while (ht->items[i] != 0) {
				hashtbl_item_t *x = ht->items[i];
				ht->items[i] = x->next;

				size_t slot = SLOT_OF_HASH(ht->hf(x->key), nsize);
				x->next = nitems[slot];
				nitems[slot] = x;
			}
		}
		free(ht->items);
		ht->size = nsize;
		ht->items = nitems;
	}
}

int hashtbl_add(hashtbl_t *ht, void* key, void* v) {
	size_t slot = SLOT_OF_HASH(ht->hf(key), ht->size);

	hashtbl_item_t *i = (hashtbl_item_t*)malloc(sizeof(hashtbl_item_t));
	if (i == 0) return 1;	// OOM

	// make sure item is not already present
	hashtbl_remove(ht, key);

	i->key = key;
	i->val = v;
	i->next = ht->items[slot];
	ht->items[slot] = i;
	ht->nitems++;

	hashtbl_check_size(ht);

	return 0;
}

void* hashtbl_find(hashtbl_t* ht, void* key) {
	size_t slot = SLOT_OF_HASH(ht->hf(key), ht->size);

	for (hashtbl_item_t *i = ht->items[slot]; i != 0; i = i->next) {
		if (ht->ef(i->key, key)) return i->val;
	}

	return 0;
}

void hashtbl_remove(hashtbl_t* ht, void* key) {
	size_t slot = SLOT_OF_HASH(ht->hf(key), ht->size);

	if (ht->items[slot] == 0) return;

	if (ht->ef(ht->items[slot]->key, key)) {
		hashtbl_item_t *x = ht->items[slot];
		ht->items[slot] = x->next;
		if (ht->kff) ht->kff(x->key);
		free(x);
		ht->nitems--;
	} else {
		for (hashtbl_item_t *i = ht->items[slot]; i->next != 0; i = i->next) {
			if (ht->ef(i->next->key, key)) {
				hashtbl_item_t *x = i->next;
				i->next = x->next;
				if (ht->kff) ht->kff(x->key);
				free(x);
				ht->nitems--;
				break;
			}
		}
	}
	
	hashtbl_check_size(ht);
}

size_t hashtbl_count(hashtbl_t* ht) {
	return ht->nitems;
}

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

/* vim: set ts=4 sw=4 tw=0 noet :*/
