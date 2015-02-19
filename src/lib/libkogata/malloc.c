#include <malloc.h>
#include <slab_alloc.h>

#include <syscall.h>
#include <user_region.h>

static void* heap_alloc_pages(size_t s) {
	void* addr = region_alloc(s, "Heap");
	if (addr == 0) return 0;

	bool map_ok = mmap(addr, s, FM_READ | FM_WRITE);
	if (!map_ok) {
		region_free(addr);
		return 0;
	}

	return addr;
}

static void heap_free_pages(void* addr) {
	munmap(addr);
	region_free(addr);
}

static mem_allocator_t *mem_allocator;
static slab_type_t slab_sizes[] = {
	{ "8B malloc objects", 8, 2 },
	{ "16B malloc objects", 16, 2 },
	{ "32B malloc objects", 32, 2 },
	{ "64B malloc objects", 64, 4 },
	{ "128B malloc objects", 128, 4 },
	{ "256B malloc objects", 256, 4 },
	{ "512B malloc objects", 512, 8 },
	{ "1KB malloc objects", 1024, 8 },
	{ "2KB malloc objects", 2048, 16 },
	{ "4KB malloc objects", 4096, 16 },
	{ 0, 0, 0 }
};

void malloc_setup() {
	region_allocator_init((void*)0x40000000, (void*)0xB0000000);

	mem_allocator = create_slab_allocator(slab_sizes, heap_alloc_pages, heap_free_pages);

	ASSERT(mem_allocator != 0);
}

void* malloc(size_t size) {
	return slab_alloc(mem_allocator, size);
}

void free(void* ptr) {
	slab_free(mem_allocator, ptr);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
