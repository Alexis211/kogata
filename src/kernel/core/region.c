#include <region.h>
#include <frame.h>
#include <paging.h>

bool alloc_map_single_frame(void* addr) {
	int frame = frame_alloc(1);
	if (frame == 0) return false;

	if (!pd_map_page(addr, frame, true)) {
		frame_free(frame, 1);
		return false;
	}

	return true;
}

// ========================================================= //
// HELPER FUNCTIONS : SIMPLE PF HANDLERS ; FREEING FUNCTIONS //
// ========================================================= //

void region_free_unmap_free(void* ptr) {
	region_info_t *i = find_region(ptr);
	ASSERT(i != 0);

	for (void* x = i->addr; x < i->addr + i->size; x += PAGE_SIZE) {
		uint32_t f = pd_get_frame(x);
		if (f != 0) {
			pd_unmap_page(x);
			frame_free(f, 1);
		}
	}
	region_free(ptr);
}

void region_free_unmap(void* ptr) {
	region_info_t *i = find_region(ptr);
	ASSERT(i != 0);

	for (void* x = i->addr; x < i->addr + i->size; x += PAGE_SIZE) {
		pd_unmap_page(x);
	}
	region_free(ptr);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
