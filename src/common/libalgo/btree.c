#include <malloc.h>
#include <debug.h>

#include <btree.h>

typedef struct btree_item {
	void *key, *val;

	int height;
	struct btree_item *left, *right;
} btree_item_t;

struct btree {
	key_cmp_fun_t cf;
	kv_iter_fun_t releasef;

	btree_item_t *root;
	int nitems;
};

btree_t* create_btree(key_cmp_fun_t cf, kv_iter_fun_t relf) {
	btree_t* t = (btree_t*)malloc(sizeof(btree_t));

	if (t == 0) return 0;

	t->cf = cf;
	if (t->releasef) t->releasef = relf;

	t->root = 0;
	t->nitems = 0;

	return t;
}

void delete_btree(btree_t *t) {
	void delete_item_rec(btree_item_t *i,kv_iter_fun_t relf) {
		if (i == 0) return;

		delete_item_rec(i->left, relf);
		delete_item_rec(i->right, relf);

		if (relf) relf(i->key, i->val);
		free(i);
	}

	delete_item_rec(t->root, t->releasef);

	free(t);
}

size_t btree_count(btree_t *i) {
	return i->nitems;
}

// ============== //
// TREE ROTATIONS //
// ============== //

static inline int height(btree_item_t *i) {
	if (i == 0) return 0;
	return i->height;
}

void btree_recalc_height(btree_item_t *i) {
	if (i == 0) return;

	i->height = MAX(height(i->left), height(i->right)) + 1;
}

btree_item_t* btree_rotate_left(btree_item_t *i) {
	// I(X(a, b), Y) -> a X b I Y -> X(a, I(b, Y))

	btree_item_t *x = i->left;
	if (x == 0) return i;
	btree_item_t *b = x->right;

	x->right = i;
	i->left = b;

	btree_recalc_height(i);
	btree_recalc_height(x);

	return x;
}

btree_item_t* btree_rotate_right(btree_item_t *i) {
	// I(X, Y(a,b)) -> X I a Y b -> Y(I(X, a), b)

	btree_item_t *y = i->right;
	if (y == 0) return i;
	btree_item_t *a = y->left;

	y->left = i;
	i->right = a;

	btree_recalc_height(i);
	btree_recalc_height(y);

	return y;
}

btree_item_t* btree_equilibrate(btree_item_t *i) {
	if (i == 0) return 0;

	while (height(i->left) - height(i->right) >= 2)
		i = btree_rotate_left(i);

	while (height(i->right) - height(i->left) >= 2)
		i = btree_rotate_right(i);
	
	return i;
}

// =================== //
// ADDING AND DELETING //
// =================== //

bool btree_add(btree_t *t, void* key, void* val) {
	btree_item_t* insert(btree_t *t, btree_item_t *root, btree_item_t *i) {
		if (root == 0) return i;

		if (t->cf(i->key, root->key) <= 0) {
			root->left = insert(t, root->left, i);
		} else {
			root->right = insert(t, root->right, i);
		}
		btree_recalc_height(root);

		return btree_equilibrate(root);
	}

	btree_item_t *x = (btree_item_t*)malloc(sizeof(btree_item_t));
	if (x == 0) return false;

	x->key = key;
	x->val = val;
	x->left = x->right = 0;
	btree_recalc_height(x);

	t->root = insert(t, t->root, x);
	t->nitems++;

	return true;
}

void btree_remove(btree_t *t, const void* key) {
	btree_item_t *extract_smallest(btree_item_t *i, btree_item_t **out_smallest) {
		ASSERT(i != 0);
		if (i->left == 0) {
			*out_smallest = i;
			return i->right;
		} else {
			i->left = extract_smallest(i->left, out_smallest);
			btree_recalc_height(i);
			return btree_equilibrate(i);
		}
	}

	btree_item_t *remove_aux(btree_t *t, btree_item_t *i, const void* key) {
		if (i == 0) return 0;

		int c = t->cf(key, i->key);
		if (c < 0) {
			i->left = remove_aux(t, i->left, key);
			return btree_equilibrate(i);
		} else if (c > 0) {
			i->right = remove_aux(t, i->right, key);
			return btree_equilibrate(i);
		} else {
			// remove item i

			btree_item_t *new_i;
			if (i->right == 0 || i->left == 0) {
				new_i = (i->right == 0 ? i->left : i->right);
			} else {
				btree_item_t *new_i_right = extract_smallest(i->right, &new_i);

				new_i->left = i->left;
				new_i->right = new_i_right;

				btree_recalc_height(new_i);
				new_i = btree_equilibrate(new_i);
			}

			if (t->releasef) t->releasef(i->key, i->val);
			free(i);
			t->nitems--;

			return remove_aux(t, new_i, key);	// loop because several elements may correspond
		}
	}

	t->root = remove_aux(t, t->root, key);
}

void btree_remove_v(btree_t *t, const void* key, const void* val) {
	btree_item_t *extract_smallest(btree_item_t *i, btree_item_t **out_smallest) {
		ASSERT(i != 0);
		if (i->left == 0) {
			*out_smallest = i;
			return i->right;
		} else {
			i->left = extract_smallest(i->left, out_smallest);
			btree_recalc_height(i);
			return btree_equilibrate(i);
		}
	}

	btree_item_t *remove_aux(btree_t *t, btree_item_t *i, const void* key, const void* val) {
		if (i == 0) return 0;

		int c = t->cf(key, i->key);
		if (c < 0) {
			i->left = remove_aux(t, i->left, key, val);
			return btree_equilibrate(i);
		} else if (c > 0) {
			i->right = remove_aux(t, i->right, key, val);
			return btree_equilibrate(i);
		} else if (i->val == val) {
			// remove item i

			btree_item_t *new_i;
			if (i->right == 0 || i->left == 0) {
				new_i = (i->right == 0 ? i->left : i->right);
			} else {
				btree_item_t *new_i_right = extract_smallest(i->right, &new_i);

				new_i->left = i->left;
				new_i->right = new_i_right;

				btree_recalc_height(new_i);
				new_i = btree_equilibrate(new_i);
			}

			if (t->releasef) t->releasef(i->key, i->val);
			free(i);
			t->nitems--;

			return remove_aux(t, new_i, key, val);	// loop because several elements may correspond
		} else {
			i->left = remove_aux(t, i->left, key, val);
			i->right = remove_aux(t, i->right, key, val);
			btree_recalc_height(i);
			return btree_equilibrate(i);
		}
	}

	t->root = remove_aux(t, t->root, key, val);
}

// ======================== //
// LOOKING UP AND ITERATING //
// ======================== //

void* btree_find(btree_t *t, const void* key) {
	btree_item_t *find_aux(btree_t *t, btree_item_t *i, const void* key) {
		if (i == 0) return 0;

		int c = t->cf(key, i->key);
		if (c == 0) {
			return i;
		} else if (c < 0) {
			return find_aux(t, i->left, key);
		} else {
			return find_aux(t, i->right, key);
		}
	}

	btree_item_t *i = find_aux(t, t->root, key);

	if (i == 0) return 0;
	return i->val;
}

void* btree_lower(btree_t *t, const void* key) {
	btree_item_t *find_aux(btree_t *t, btree_item_t *i, const void* key) {
		if (i == 0) return 0;

		int c = t->cf(key, i->key);
		if (c == 0) {
			return i;
		} else if (c < 0) {
			return find_aux(t, i->left, key);
		} else {
			btree_item_t *r = find_aux(t, i->right, key);
			if (r == 0) r = i;
			return r;
		}
	}

	btree_item_t *i = find_aux(t, t->root, key);

	if (i == 0) return 0;
	return i->val;
}

void* btree_upper(btree_t *t, const void* key) {
	btree_item_t *find_aux(btree_t *t, btree_item_t *i, const void* key) {
		if (i == 0) return 0;

		int c = t->cf(key, i->key);
		if (c == 0) {
			return i;
		} else if (c < 0) {
			btree_item_t *r = find_aux(t, i->left, key);
			if (r == 0) r = i;
			return r;
		} else {
			return find_aux(t, i->right, key);
		}
	}

	btree_item_t *i = find_aux(t, t->root, key);

	if (i == 0) return 0;
	return i->val;
}


void btree_iter(btree_t *t, kv_iter_fun_t f) {
	void iter_aux(btree_item_t *i, kv_iter_fun_t f) {
		if (i == 0) return;

		iter_aux(i->left, f);
		f(i->key, i->val);
		iter_aux(i->right, f);
	}

	iter_aux(t->root, f);
}

void btree_iter_on(btree_t *t, const void* key, kv_iter_fun_t f) {
	void iter_aux(btree_t *t, btree_item_t *i, const void* key, kv_iter_fun_t f) {
		if (i == 0) return;

		int c = t->cf(key, i->key);
		if (c == 0) {
			iter_aux(t, i->left, key, f);
			f(i->key, i->val);
			iter_aux(t, i->right, key, f);
		} else if (c < 0) {
			iter_aux(t, i->left, key, f);
		} else {
			iter_aux(t, i->right, key, f);
		}
	}

	iter_aux(t, t->root, key, f);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
