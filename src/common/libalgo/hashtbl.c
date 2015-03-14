#include <debug.h>
#include <malloc.h>

#include <hashtbl.h>

#define DEFAULT_HT_INIT_SIZE 16
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
	kv_iter_fun_t releasef;
	size_t size, nitems;
	hashtbl_item_t **items;
};

hashtbl_t *create_hashtbl(key_eq_fun_t ef, hash_fun_t hf, kv_iter_fun_t on_release) {
	hashtbl_t *ht = (hashtbl_t*)malloc(sizeof(hashtbl_t));
	if (ht == 0) return 0;

	ht->ef = ef;
	ht->hf = hf;
	ht->releasef = on_release;

	ht->size = DEFAULT_HT_INIT_SIZE;
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
			hashtbl_item_t *x = ht->items[i];
			ht->items[i] = x->next;

			if (ht->releasef) ht->releasef(x->key, x->val);
			
			free(x);
		}
	}

	// Free table
	free(ht->items);
	free(ht);
}

void hashtbl_check_size(hashtbl_t *ht) {
	size_t nsize = 0;
	if (4 * ht->nitems < ht->size) nsize = ht->size / 2;
	if (4 * ht->nitems > 3 * ht->size) nsize = ht->size * 2;

	if (nsize != 0) {
		hashtbl_item_t **nitems = (hashtbl_item_t**)malloc(nsize * sizeof(hashtbl_item_t*));
		if (nitems == 0) return;	// if we can't realloc, too bad, we just lose space/efficienty

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

bool hashtbl_add(hashtbl_t *ht, void* key, void* v) {
	hashtbl_item_t *i = (hashtbl_item_t*)malloc(sizeof(hashtbl_item_t));
	if (i == 0) return false;	// OOM

	// make sure item is not already present
	hashtbl_remove(ht, key);

	size_t slot = SLOT_OF_HASH(ht->hf(key), ht->size);

	i->key = key;
	i->val = v;
	i->next = ht->items[slot];
	ht->items[slot] = i;
	ht->nitems++;

	hashtbl_check_size(ht);

	return true;
}

void hashtbl_remove(hashtbl_t* ht, const void* key) {
	size_t slot = SLOT_OF_HASH(ht->hf(key), ht->size);

	if (ht->items[slot] == 0) return;

	hashtbl_item_t *x = 0;

	if (ht->ef(ht->items[slot]->key, key)) {
		x = ht->items[slot];
		ht->items[slot] = x->next;
	} else {
		for (hashtbl_item_t *i = ht->items[slot]; i->next != 0; i = i->next) {
			if (ht->ef(i->next->key, key)) {
				x = i->next;
				i->next = x->next;
				break;
			}
		}
	}

	if (x != 0) {
		ht->nitems--;
		if (ht->releasef) ht->releasef(x->key, x->val);
		free(x);
	}
	
	hashtbl_check_size(ht);
}

void* hashtbl_find(hashtbl_t* ht, const void* key) {
	size_t slot = SLOT_OF_HASH(ht->hf(key), ht->size);

	for (hashtbl_item_t *i = ht->items[slot]; i != 0; i = i->next) {
		if (ht->ef(i->key, key)) return i->val;
	}

	return 0;
}

void hashtbl_iter(hashtbl_t *ht, kv_iter_fun_t f) {
	for (size_t s = 0; s < ht->size; s++) {
		for (hashtbl_item_t *i = ht->items[s]; i != 0; i = i->next) {
			f(i->key, i->val);
		}
	}
}

size_t hashtbl_count(hashtbl_t* ht) {
	return ht->nitems;
}

bool hashtbl_change(hashtbl_t* ht, void* key, void* newval) {
	size_t slot = SLOT_OF_HASH(ht->hf(key), ht->size);

	for (hashtbl_item_t *i = ht->items[slot]; i != 0; i = i->next) {
		if (ht->ef(i->key, key)) {
			i->val = newval;
			return true;
		}
	}

	return false;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
