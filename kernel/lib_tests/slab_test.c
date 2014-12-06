#include <slab_alloc.h>
#include <stdlib.h>
#include <stdio.h>

slab_type_t slab_sizes[] = {
	{ "8B obj", 8, 1 },
	{ "16B obj", 16, 2 },
	{ "32B obj", 32, 2 },
	{ "64B obj", 64, 2 },
	{ "128B obj", 128, 2 },
	{ "256B obj", 256, 4 },
	{ "512B obj", 512, 4 },
	{ "1KB obj", 1024, 8 },
	{ "2KB obj", 2048, 8 },
	{ "4KB obj", 4096, 16 },
	{ 0, 0, 0 }
};


int main(int argc, char *argv[]) {
	mem_allocator_t *a =
		create_slab_allocator(slab_sizes, malloc, free);

	const int m = 10000;
	uint32_t* ptr[m];
	for (int i = 0; i < m; i++) {
		size_t s = 1 << ((i * 7) % 12 + 2);
		ptr[i] = (uint32_t*)slab_alloc(a, s);
		ASSERT(ptr[i] != 0);
		*ptr[i] = ((i * 2117) % (1<<30));
		printf("Alloc %i : 0x%p\n", s, ptr[i]);
	}
	for (int i = 0; i < m; i++) {
		for (int j = i; j < m; j++) {
			ASSERT(*ptr[j] == (j * 2117) % (1<<30));
		}
		slab_free(a, ptr[i]);
	}
	destroy_slab_allocator(a);

	return 0;
}
