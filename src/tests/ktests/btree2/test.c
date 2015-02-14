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
		ASSERT(btree_lower(ht, (void*)k[i]) == (void*)v[i]);
		ASSERT(btree_upper(ht, (void*)k[i]) == (void*)v[i]);
	}

	// random lower bound/upper bound tests
	for (int t = 0; t < 100; t++) {
		const uint32_t xk = prng();
		int ilower = -1, iupper = -1;
		for (int i = 1; i < n; i++) {
			if (k[i] <= xk && (ilower == -1 || k[i] > k[ilower])) ilower = i;
			if (k[i] >= xk && (iupper == -1 || k[i] < k[iupper])) iupper = i;
		}
		dbg_printf("random %d : lower %d (= %d), upper %d (= %d) ; ",
			xk, k[ilower], v[ilower], k[iupper], v[iupper]);
		const uint32_t bt_lower = (uint32_t)btree_lower(ht, (void*)xk);
		const uint32_t bt_upper = (uint32_t)btree_upper(ht, (void*)xk);
		dbg_printf("got lower %d, upper %d\n", bt_lower, bt_upper);
		ASSERT(bt_lower == (ilower == -1 ? 0 : v[ilower]));;
		ASSERT(bt_upper == (iupper == -1 ? 0 : v[iupper]));;
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
