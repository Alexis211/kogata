#include <kmalloc.h>

#include <slab_alloc.h>
#include <mutex.h>

#include <frame.h>
#include <paging.h>
#include <region.h>

static void* page_alloc_fun_for_kmalloc(size_t bytes) {
	void* addr = region_alloc(bytes, "Core kernel heap", default_allocator_pf_handler);
	return addr;
}

static slab_type_t slab_sizes[] = {
	{ "8B kmalloc objects", 8, 2 },
	{ "16B kmalloc objects", 16, 2 },
	{ "32B kmalloc objects", 32, 2 },
	{ "64B kmalloc objects", 64, 4 },
	{ "128B kmalloc objects", 128, 4 },
	{ "256B kmalloc objects", 256, 4 },
	{ "512B kmalloc objects", 512, 8 },
	{ "1KB kmalloc objects", 1024, 8 },
	{ "2KB kmalloc objects", 2048, 16 },
	{ "4KB kmalloc objects", 4096, 16 },
	{ 0, 0, 0 }
};

static mem_allocator_t *kernel_allocator = 0;
STATIC_MUTEX(kmalloc_mutex);

void kmalloc_setup() {
	kernel_allocator =
	  create_slab_allocator(slab_sizes, page_alloc_fun_for_kmalloc,
										region_free_unmap_free);
}

void* kmalloc(size_t sz) {
	void* res = 0;

	mutex_lock(&kmalloc_mutex);
	res = slab_alloc(kernel_allocator, sz);
	mutex_unlock(&kmalloc_mutex);

	return res;
}

void kfree(void* ptr) {
	mutex_lock(&kmalloc_mutex);
	slab_free(kernel_allocator, ptr);
	mutex_unlock(&kmalloc_mutex);
}
