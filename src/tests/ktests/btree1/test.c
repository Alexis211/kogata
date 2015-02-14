#include <btree.h>

void test_btree_1() {
	BEGIN_TEST("test-btree-1");

	// hashtable test
	btree_t *ht = create_btree(str_key_cmp_fun, 0);
	ASSERT(ht != 0);

	ASSERT(btree_add(ht, "test1", "STRTEST1"));
	ASSERT(btree_add(ht, "test2", "STRTEST2"));
	ASSERT(btree_find(ht, "test1") != 0 && 
			strcmp(btree_find(ht, "test1"), "STRTEST1") == 0);
	ASSERT(btree_find(ht, "test2") != 0 &&
			strcmp(btree_find(ht, "test2"), "STRTEST2") == 0);
	ASSERT(btree_find(ht, "test") == 0);

	ASSERT(btree_add(ht, "test", "Forever alone"));
	ASSERT(btree_find(ht, "test1") != 0 &&
			strcmp(btree_find(ht, "test1"), "STRTEST1") == 0);
	ASSERT(btree_find(ht, "test2") != 0 &&
			strcmp(btree_find(ht, "test2"), "STRTEST2") == 0);
	ASSERT(btree_find(ht, "test") != 0 &&
			strcmp(btree_find(ht, "test"), "Forever alone") == 0);

	btree_remove(ht, "test1");
	ASSERT(btree_find(ht, "test1") == 0);
	ASSERT(btree_find(ht, "test2") != 0 &&
			strcmp(btree_find(ht, "test2"), "STRTEST2") == 0);
	ASSERT(btree_find(ht, "test") != 0 &&
			strcmp(btree_find(ht, "test"), "Forever alone") == 0);

	delete_btree(ht);

	TEST_OK;
}

#undef TEST_PLACEHOLDER_AFTER_TASKING
#define TEST_PLACEHOLDER_AFTER_TASKING { test_btree_1(); }

/* vim: set ts=4 sw=4 tw=0 noet :*/
