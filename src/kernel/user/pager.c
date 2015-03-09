#include <string.h>

#include <pager.h>
#include <vfs.h>
#include <process.h>
#include <frame.h>

#define ENT_FRAME_SHIFT 12


// ========== //
// SWAP PAGER //
// ========== //

static void swap_page_in(pager_t *p, size_t offset, size_t len);
static void swap_page_out(pager_t *p, size_t offset, size_t len);
static void swap_page_release(pager_t *p, size_t offset, size_t len);
static bool swap_resize(pager_t *p, size_t new_size);

static pager_ops_t swap_pager_ops = {
	.page_in = swap_page_in,
	.page_commit = 0,
	.page_out = swap_page_out,
	.page_release = swap_page_release,
	.resize = swap_resize,
};

pager_t *new_swap_pager(size_t size) {
	pager_t *p = (pager_t*)malloc(sizeof(pager_t));
	if (p == 0) return 0;

	p->pages = create_hashtbl(id_key_eq_fun, id_hash_fun, 0);
	if (p->pages == 0) goto error;

	p->ops = &swap_pager_ops;
	p->size = size;
	p->lock = MUTEX_UNLOCKED;
	p->maps = 0;

	return p;

error:
	if (p) free(p);
	return 0;
}

void swap_page_in(pager_t *p, size_t offset, size_t len) {
	ASSERT(PAGE_ALIGNED(offset));

	void *region = region_alloc(PAGE_SIZE, "Page zeroing area", 0);
	if (region == 0) return;

	for (size_t page = offset; page < offset + len; page += PAGE_SIZE) {
		if (hashtbl_find(p->pages, (void*)page) == 0) {
			uint32_t frame = frame_alloc(1);
			if (frame != 0) {
				if (!pd_map_page(region, frame, true)) PANIC("TODO");;
				memset(region, 0, PAGE_SIZE);
				pd_unmap_page(region);
			}
			if (!hashtbl_add(p->pages, (void*)page, (void*)(frame << ENT_FRAME_SHIFT))) {
				frame_free(frame, 1);
			}
		}
	}

	region_free(region);
}

void swap_page_out(pager_t *p, size_t offset, size_t len) {
	// later : save to swap file...
}

void swap_page_release(pager_t *p, size_t offset, size_t len) {
	ASSERT(PAGE_ALIGNED(offset));

	for (size_t page = offset; page < offset + len; page += PAGE_SIZE) {
		uint32_t frame = (uint32_t)hashtbl_find(p->pages, (void*)page) >> ENT_FRAME_SHIFT;
		if (frame != 0) {
			hashtbl_remove(p->pages, (void*)page);
			frame_free(frame, 1);
		}
	}
}

bool swap_resize(pager_t *p, size_t new_size) {
	// later : remove unused pages in swap file

	swap_page_release(p, PAGE_ALIGN_UP(new_size), p->size - PAGE_ALIGN_UP(new_size));

	size_t last_page = PAGE_ALIGN_DOWN(new_size);

	if (!PAGE_ALIGNED(new_size) && hashtbl_find(p->pages, (void*)last_page) != 0) {
		void *region = region_alloc(PAGE_SIZE, "Page zeroing area", 0);
		if (!region) PANIC("TODO");

		uint32_t frame = (uint32_t)hashtbl_find(p->pages, (void*)last_page) >> ENT_FRAME_SHIFT;
		ASSERT(frame != 0);

		if (!pd_map_page(region, frame, true)) PANIC("TODO");

		size_t b0 = new_size - last_page;
		memset(region + b0, 0, PAGE_SIZE - b0);

		pd_unmap_page(region);

		region_free(region);
	}

	return true;
}

// ============ //
// DEVICE PAGER //
// ============ //

static void device_page_in(pager_t *p, size_t offset, size_t len);

static pager_ops_t device_pager_ops = {
	.page_in = device_page_in,
	.page_commit = 0,
	.page_out = 0,
	.page_release = 0,
	.resize = 0,
};

pager_t *new_device_page(size_t size, size_t phys_offset) {
	ASSERT(PAGE_ALIGNED(phys_offset));

	pager_t *p = (pager_t*)malloc(sizeof(pager_t));
	if (p == 0) return 0;

	p->pages = create_hashtbl(id_key_eq_fun, id_hash_fun, 0);
	if (p->pages == 0) goto error;

	p->ops = &device_pager_ops;
	p->size = size;
	p->lock = MUTEX_UNLOCKED;
	p->maps = 0;
	p->device_pager.phys_offset = phys_offset;

	return p;

error:
	if (p) free(p);
	return 0;
}

void device_page_in(pager_t *p, size_t offset, size_t len) {
	ASSERT(PAGE_ALIGNED(offset));

	for (size_t page = offset; page < offset + len; page += PAGE_SIZE) {
		if (hashtbl_find(p->pages, (void*)page) == 0) {
			hashtbl_add(p->pages, (void*)page, (void*)(p->device_pager.phys_offset + page));
		}
	}
}

// ======================= //
// GENERIC PAGER FUNCTIONS //
// ======================= //

void delete_pager(pager_t *p) {
	ASSERT(p->maps == 0);

	mutex_lock(&p->lock);

	if (p->ops->page_commit) p->ops->page_commit(p, 0, p->size);
	if (p->ops->page_release) p->ops->page_release(p, 0, p->size);

	delete_hashtbl(p->pages);
	free(p);
}

bool pager_resize(pager_t *p, size_t newsize) {
	if (!p->ops->resize) return false;

	mutex_lock(&p->lock);

	bool ret = p->ops->resize(p, newsize);

	mutex_unlock(&p->lock);
	
	return ret;
}

void pager_access(pager_t *p, size_t offset, size_t len, bool accessed, bool dirty) {
	mutex_lock(&p->lock);

	ASSERT(PAGE_ALIGNED(offset));

	for (size_t page = offset; page < offset + len; page += PAGE_SIZE) {
		uint32_t v = (uint32_t)hashtbl_find(p->pages, (void*)page);
		if (v != 0) {
			hashtbl_change(p->pages, (void*)page,
				(void*)(v | (accessed ? PAGE_ACCESSED : 0) | (dirty ? PAGE_DIRTY : 0)));
		}
	}

	mutex_unlock(&p->lock);
}

void pager_commit_dirty(pager_t *p) {
	if (!p->ops->page_commit) return;

	mutex_lock(&p->lock);

	p->ops->page_commit(p, 0, p->size);

	mutex_unlock(&p->lock);
}

int pager_get_frame(pager_t *p, size_t offset) {
	mutex_lock(&p->lock);

	ASSERT(PAGE_ALIGNED(offset));

	uint32_t v = (uint32_t)hashtbl_find(p->pages, (void*)offset);
	if (v == 0) {
		p->ops->page_in(p, offset, PAGE_SIZE);
	}

	uint32_t ret = (uint32_t)hashtbl_find(p->pages, (void*)offset) >> ENT_FRAME_SHIFT;

	mutex_unlock(&p->lock);

	return ret;
}

static size_t pager_do_rw(pager_t *p, size_t offset, size_t len, char* buf, bool write) {
	size_t ret = 0;
	void* region = 0;

	mutex_lock(&p->lock);

	if (offset >= p->size) return 0;
	if (offset + len > p->size) len = p->size - offset;

	size_t first_page = PAGE_ALIGN_DOWN(offset);
	size_t region_len = offset + len - first_page;

	region = region_alloc(PAGE_ALIGN_UP(region_len), "Temporary pager read/write zone", 0);
	if (region == 0) goto end_read;

	p->ops->page_in(p, first_page, region_len);
	for (size_t i = first_page; i < first_page + region_len; i += PAGE_SIZE) {
		uint32_t frame = (uint32_t)hashtbl_find(p->pages, (void*)i) >> ENT_FRAME_SHIFT;
		if (frame == 0) goto end_read;
		if (!pd_map_page(region + i - first_page, frame, write)) goto end_read;
	}

	if (write) {
		memcpy(region + offset - first_page, buf, len);
		for (size_t page = first_page; page < first_page+ region_len; page += PAGE_SIZE) {
			uint32_t v = (uint32_t)hashtbl_find(p->pages, (void*)page);
			ASSERT(v != 0);
			hashtbl_change(p->pages, (void*)page, (void*)(v | PAGE_ACCESSED));
		}
	} else {
		memcpy(buf, region + offset - first_page, len);
	}
	ret = len;

end_read:
	if (region) {
		for (void* x = region; x < region + region_len; x += PAGE_SIZE) {
			if (pd_get_frame(x) != 0) pd_unmap_page(x);
		}
		region_free(region);
	}
	mutex_unlock(&p->lock);
	return ret;
}

size_t pager_read(pager_t *p, size_t offset, size_t len, char* buf) {
	return pager_do_rw(p, offset, len, buf, false);
}

size_t pager_write(pager_t *p, size_t offset, size_t len, const char* buf) {
	return pager_do_rw(p, offset, len, (char*)buf, true);
}

void pager_add_map(pager_t *p, user_region_t *reg) {
	mutex_lock(&p->lock);

	reg->next_for_pager = p->maps;
	p->maps = reg;

	mutex_unlock(&p->lock);
}

void pager_rm_map(pager_t *p, user_region_t *reg) {
	mutex_lock(&p->lock);

	if (p->maps == reg) {
		p->maps = reg->next_for_pager;
	} else {
		for (user_region_t *it = p->maps; it->next_for_pager != 0; it = it->next_for_pager) {
			if (it->next_for_pager == reg) {
				it->next_for_pager = reg->next_for_pager;
				break;
			}
		}
	}
	reg->next_for_pager = 0;

	mutex_unlock(&p->lock);
}

//  ---- For use by FS drivers

size_t fs_read_from_pager(fs_handle_t *f, size_t offset, size_t len, char* buf) {
	ASSERT(f->node->pager != 0);

	return pager_read(f->node->pager, offset, len, buf);
}
size_t fs_write_to_pager(fs_handle_t *f, size_t offset, size_t len, const char* buf) {
	ASSERT(f->node->pager != 0);
	if (offset + len > f->node->pager->size) pager_resize(f->node->pager, offset + len);

	return pager_write(f->node->pager, offset, len, buf);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
