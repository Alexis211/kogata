#include <hashtbl.h>

void test_hashtbl_2() {
	BEGIN_TEST("test-hashtbl-2");

	hashtbl_t *ht = create_hashtbl(id_key_eq_fun, id_hash_fun, 0);
	ASSERT(ht != 0);

	ASSERT(hashtbl_add(ht, (void*)12, "TESTSTR12"));
	ASSERT(hashtbl_add(ht, (void*)777, "TESTSTR777"));

	ASSERT(hashtbl_find(ht, (void*)12) != 0 &&
			strcmp(hashtbl_find(ht, (void*)12), "TESTSTR12") == 0);
	ASSERT(hashtbl_find(ht, (void*)777) != 0 &&
			strcmp(hashtbl_find(ht, (void*)777), "TESTSTR777") == 0);
	ASSERT(hashtbl_find(ht, (void*)144) == 0);

	ASSERT(hashtbl_add(ht, (void*)144, "Forever alone"));

	ASSERT(hashtbl_find(ht, (void*)12) != 0 &&
			strcmp(hashtbl_find(ht, (void*)12), "TESTSTR12") == 0);
	ASSERT(hashtbl_find(ht, (void*)144) != 0 &&
			strcmp(hashtbl_find(ht, (void*)144), "Forever alone") == 0);
	ASSERT(hashtbl_find(ht, (void*)777) != 0 &&
			strcmp(hashtbl_find(ht, (void*)777), "TESTSTR777") == 0);

	hashtbl_remove(ht, (void*)12);
	ASSERT(hashtbl_find(ht, (void*)12) == 0);
	ASSERT(hashtbl_find(ht, (void*)144) != 0 &&
			strcmp(hashtbl_find(ht, (void*)144), "Forever alone") == 0);
	ASSERT(hashtbl_find(ht, (void*)777) != 0 &&
			strcmp(hashtbl_find(ht, (void*)777), "TESTSTR777") == 0);

	delete_hashtbl(ht);

	TEST_OK;
}

#undef TEST_PLACEHOLDER_AFTER_TASKING
#define TEST_PLACEHOLDER_AFTER_TASKING { test_hashtbl_2(); }

/* vim: set ts=4 sw=4 tw=0 noet :*/
