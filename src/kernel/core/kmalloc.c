#include <kmalloc.h>

#include <slab_alloc.h>
#include <mutex.h>

#include <frame.h>
#include <paging.h>
#include <region.h>
#include <freemem.h>

void* page_alloc_fun_for_kmalloc(size_t bytes) {
	void* addr = region_alloc(bytes, "Core kernel heap");
	if (addr == 0) {
		if (SPAM_OOM_REASON) {
			dbg_printf("Could not allocate region for kmalloc.\n");
			dbg_print_frame_stats();
			dbg_print_region_info();
		}
		return 0;
	}

	// Map physical memory
	for (void* i = addr; i < addr + bytes; i += PAGE_SIZE) {
		int f = frame_alloc(1);
		if (f == 0) goto failure;
		if (!pd_map_page(i, f, true)) {
			frame_free(f, 1);
			goto failure;
		}
	}

	return addr;

failure:
	for (void* i = addr; i < addr + bytes; i += PAGE_SIZE) {
		int f = pd_get_frame(i);
		if (f != 0) {
			pd_unmap_page(i);
			frame_free(f, 1);
		}
	}
	region_free(addr);
	return 0;
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
STATIC_MUTEX(malloc_mutex);

void kmalloc_setup() {
	kernel_allocator =
	  create_slab_allocator(slab_sizes, page_alloc_fun_for_kmalloc,
										region_free_unmap_free);
}

void* malloc0(size_t sz) {
	void* res = 0;

	mutex_lock(&malloc_mutex);
	res = slab_alloc(kernel_allocator, sz);
	mutex_unlock(&malloc_mutex);

	return res;
}

void* malloc(size_t sz) {
	void* res;
	int tries = 0;

	while ((res = malloc0(sz)) == 0 && (tries++) < 3) {
		if (SPAM_OOM_REASON) dbg_printf("OOM in kmalloc\n");
		free_some_memory();
	}

	return res;
}

void free(void* ptr) {
	mutex_lock(&malloc_mutex);
	slab_free(kernel_allocator, ptr);
	mutex_unlock(&malloc_mutex);
}
