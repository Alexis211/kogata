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

static pager_ops_t swap_pager_ops = {
	.page_in = swap_page_in,
	.page_commit = 0,
	.page_out = swap_page_out,
	.page_release = swap_page_release,
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
	size_t npages = PAGE_ALIGN_UP(len) / PAGE_SIZE;

	void *region = region_alloc(PAGE_SIZE, "Page zero area", 0);
	if (region == 0) return;

	for (size_t i = 0; i < npages; i++) {
		size_t page = offset + (i * PAGE_SIZE);
		if (hashtbl_find(p->pages, (void*)page) == 0) {
			uint32_t frame = frame_alloc(1);
			if (frame != 0) {
				pd_map_page(region, frame, true);
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
	size_t npages = PAGE_ALIGN_UP(len) / PAGE_SIZE;

	for (size_t i = 0; i < npages; i++) {
		size_t page = offset + (i * PAGE_SIZE);
		uint32_t frame = (uint32_t)hashtbl_find(p->pages, (void*)page) >> ENT_FRAME_SHIFT;
		if (frame != 0) {
			hashtbl_remove(p->pages, (void*)page);
			frame_free(frame, 1);
		}
	}
}

// ======================= //
// GENERIC PAGER FUNCTIONS //
// ======================= //

void delete_pager(pager_t *p) {
	ASSERT(p->maps == 0);

	pager_commit_dirty(p);

	mutex_lock(&p->lock);

	p->ops->page_release(p, 0, p->size);
	delete_hashtbl(p->pages);
	free(p);
}

void pager_access(pager_t *p, size_t offset, size_t len, bool accessed, bool dirty) {
	mutex_lock(&p->lock);

	ASSERT(PAGE_ALIGNED(offset));
	size_t npages = PAGE_ALIGN_UP(len) / PAGE_SIZE;

	for (size_t i = 0; i < npages; i++) {
		size_t page = offset + (i * PAGE_SIZE);
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

	for (size_t i = 0; i < p->size; i += PAGE_SIZE) {
		uint32_t v = (uint32_t)hashtbl_find(p->pages, (void*)i);
		if (v & PAGE_DIRTY) {
			p->ops->page_commit(p, i, PAGE_SIZE);
			hashtbl_change(p->pages, (void*)i, (void*)(v & ~PAGE_DIRTY));
		}

	}

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

/* vim: set ts=4 sw=4 tw=0 noet :*/
