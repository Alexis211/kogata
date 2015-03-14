#include <frame.h>
#include <dbglog.h>

#include <mutex.h>

// TODO: buddy allocator
// this is a simple bitmap allocator
	
#define INDEX_FROM_BIT(a) ((a)/(8*4))
#define OFFSET_FROM_BIT(a) ((a)%(8*4))

static uint32_t *frame_bitset;
static uint32_t nframes, nused_frames;
static uint32_t begin_search_at;

void frame_init_allocator(size_t total_ram, void** kernel_data_end) {
	nframes = PAGE_ID(total_ram);

	frame_bitset = (uint32_t*)ALIGN4_UP((size_t)*kernel_data_end);
	*kernel_data_end = (void*)frame_bitset + ALIGN4_UP(nframes / 8);

	for (size_t i = 0; i < ALIGN4_UP(nframes / 8)/4; i++)
		frame_bitset[i] = 0;

	nused_frames = 0;

	size_t kernel_pages = PAGE_ALIGN_UP((size_t)*kernel_data_end - K_HIGHHALF_ADDR)/PAGE_SIZE;
	for (size_t i = 0; i < kernel_pages; i++) {
		size_t idx = INDEX_FROM_BIT(i);
		size_t ofs = OFFSET_FROM_BIT(i);
		frame_bitset[idx] |= (0x1 << ofs);
		nused_frames++;
	}
	begin_search_at = INDEX_FROM_BIT(kernel_pages);
}

STATIC_MUTEX(frame_allocator_mutex);

uint32_t frame_alloc(size_t n) {
	if (n > 32) return 0;

	mutex_lock(&frame_allocator_mutex);
	for (uint32_t i = begin_search_at; i < INDEX_FROM_BIT(nframes); i++) {
		if (frame_bitset[i] == 0xFFFFFFFF) {
			if (i == begin_search_at) begin_search_at++;
			continue;
		}

		for (uint32_t j = 0; j < 32 - n + 1; j++) {
			uint32_t to_test = (0xFFFFFFFF >> (32 - n)) << j;
			if (!(frame_bitset[i]&to_test)) {
				frame_bitset[i] |= to_test;
				nused_frames += n;

				mutex_unlock(&frame_allocator_mutex);
				/*dbg_printf("AF 0x%p\n", i * 32 + j);*/

				return i * 32 + j;
			}
		}
	}
	mutex_unlock(&frame_allocator_mutex);
	return 0;
}

void frame_free(uint32_t base, size_t n) {
	mutex_lock(&frame_allocator_mutex);

	for (size_t i = 0; i < n; i++) {
		/*dbg_printf("FF 0x%p\n", base + i);*/
		uint32_t idx = INDEX_FROM_BIT(base + i);
		uint32_t ofs = OFFSET_FROM_BIT(base + i);
		if (frame_bitset[idx] & (0x1 << ofs)) {
			frame_bitset[idx] &= ~(0x1 << ofs);
			nused_frames--;
		}
	}
	if (INDEX_FROM_BIT(base) < begin_search_at)
		begin_search_at = INDEX_FROM_BIT(base);

	mutex_unlock(&frame_allocator_mutex);
}

void dbg_print_frame_stats() {
	dbg_printf("Used frames: %d/%d\n", nused_frames, nframes);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
