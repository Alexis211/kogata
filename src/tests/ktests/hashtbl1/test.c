#include <hashtbl.h>

void test_hashtbl_1() {
	BEGIN_TEST("test-hashtbl-1");

	// hashtable test
	hashtbl_t *ht = create_hashtbl(str_key_eq_fun, str_hash_fun, 0);
	ASSERT(ht != 0);

	ASSERT(hashtbl_add(ht, "test1", "STRTEST1"));
	ASSERT(hashtbl_add(ht, "test2", "STRTEST2"));
	ASSERT(hashtbl_find(ht, "test1") != 0 && 
			strcmp(hashtbl_find(ht, "test1"), "STRTEST1") == 0);
	ASSERT(hashtbl_find(ht, "test2") != 0 &&
			strcmp(hashtbl_find(ht, "test2"), "STRTEST2") == 0);
	ASSERT(hashtbl_find(ht, "test") == 0);

	ASSERT(hashtbl_add(ht, "test", "Forever alone"));
	ASSERT(hashtbl_find(ht, "test1") != 0 &&
			strcmp(hashtbl_find(ht, "test1"), "STRTEST1") == 0);
	ASSERT(hashtbl_find(ht, "test2") != 0 &&
			strcmp(hashtbl_find(ht, "test2"), "STRTEST2") == 0);
	ASSERT(hashtbl_find(ht, "test") != 0 &&
			strcmp(hashtbl_find(ht, "test"), "Forever alone") == 0);

	hashtbl_remove(ht, "test1");
	ASSERT(hashtbl_find(ht, "test1") == 0);
	ASSERT(hashtbl_find(ht, "test2") != 0 &&
			strcmp(hashtbl_find(ht, "test2"), "STRTEST2") == 0);
	ASSERT(hashtbl_find(ht, "test") != 0 &&
			strcmp(hashtbl_find(ht, "test"), "Forever alone") == 0);

	delete_hashtbl(ht);

	TEST_OK;
}

#undef TEST_PLACEHOLDER_AFTER_TASKING
#define TEST_PLACEHOLDER_AFTER_TASKING { test_hashtbl_1(); }

/* vim: set ts=4 sw=4 tw=0 noet :*/
