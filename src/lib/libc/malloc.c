#include <kogata/malloc.h>
#include <kogata/slab_alloc.h>

#include <kogata/syscall.h>
#include <kogata/region_alloc.h>

#include <string.h>

static void* heap_alloc_pages(size_t s) {
	void* addr = region_alloc(s, "Heap");
	if (addr == 0) return 0;

	bool map_ok = sc_mmap(addr, s, FM_READ | FM_WRITE);
	if (!map_ok) {
		region_free(addr);
		return 0;
	}

	return addr;
}

static void heap_free_pages(void* addr) {
	sc_munmap(addr);
	region_free(addr);
}

static mem_allocator_t *mem_allocator;
static slab_type_t slab_sizes[] = {
	{ "8B malloc objects", 8, 2 },
	{ "16B malloc objects", 16, 4 },
	{ "32B malloc objects", 32, 4 },
	{ "64B malloc objects", 64, 4 },
	{ "128B malloc objects", 128, 8 },
	{ "256B malloc objects", 256, 8 },
	{ "512B malloc objects", 512, 8 },
	{ "1KB malloc objects", 1024, 16 },
	{ "2KB malloc objects", 2048, 16 },
	{ "4KB malloc objects", 4096, 16 },
	{ 0, 0, 0 }
};

bool mmap_single_page(void* addr) {
	return sc_mmap(addr, PAGE_SIZE, MM_READ | MM_WRITE);
}

void malloc_setup() {
	region_allocator_init((void*)0x40000000, (void*)0x40000000, (void*)0xB0000000, mmap_single_page);

	mem_allocator = create_slab_allocator(slab_sizes, heap_alloc_pages, heap_free_pages);

	ASSERT(mem_allocator != 0);
}

void* malloc(size_t size) {
	if (size == 0) return 0;

	//dbg_printf("malloc 0x%p -> ", size);
	void* ret = slab_alloc(mem_allocator, size);
	//dbg_printf("0x%p\n", ret);
	return ret;
}

void* calloc(size_t nmemb, size_t sz) {
	void* r = malloc(nmemb * sz);
	if (r != 0) memset(r, 0, nmemb * sz);
	return r;
}

void* realloc(void* ptr, size_t sz) {
	//dbg_printf("realloc 0x%p 0x%p -> ", ptr, sz);
	void* ret = slab_realloc(mem_allocator, ptr, sz);
	//dbg_printf("0x%p\n", ret);
	return ret;
}

void free(void* ptr) {
	if (ptr == 0) return;
	
	//dbg_printf("free 0x%p\n", ptr);
	slab_free(mem_allocator, ptr);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
