#include <btree.h>

uint32_t prng() {
	static uint32_t prng_s = 5091;
	prng_s = ((prng_s + 24162) * 15322) % 100001;
	return prng_s;
}

void test_btree_2() {
	BEGIN_TEST("test-btree-2");

	btree_t *ht = create_btree(id_key_cmp_fun, 0);
	ASSERT(ht != 0);

	const int n = 100;
	uint32_t k[n], v[n];
	for (int i = 0; i < n; i++) {
		k[i] = prng();
		v[i] = prng();
		dbg_printf("add %d -> %d\n", k[i], v[i]);
		ASSERT(btree_add(ht, (void*)k[i], (void*)v[i]));
		ASSERT(btree_find(ht, (void*)k[i]) == (void*)v[i]);
	}

	for (int i = 0; i < n; i++) {
		ASSERT(btree_find(ht, (void*)k[i]) == (void*)v[i]);
	}

	for (int i = 0; i < n/2; i++) {
		btree_remove(ht, (void*)k[i]);
		ASSERT(btree_find(ht, (void*)k[i]) == 0);
	}

	for (int i = 0; i < n; i++) {
		ASSERT(btree_find(ht, (void*)k[i]) == (i < n/2 ? 0 : (void*)v[i]));
	}

	delete_btree(ht);

	TEST_OK;
}

#undef TEST_PLACEHOLDER_AFTER_TASKING
#define TEST_PLACEHOLDER_AFTER_TASKING { test_btree_2(); }

/* vim: set ts=4 sw=4 tw=0 noet :*/
