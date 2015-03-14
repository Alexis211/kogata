#include <slab_alloc.h>

typedef struct object {
	struct object *next;
} object_t;

typedef struct cache {
	void* region_addr;
	
	uint32_t n_free_objs;
	object_t* first_free_obj;

	struct cache *next_cache;	// next cache in this slab
} cache_t;

typedef struct region {
	void* region_addr;
	size_t region_size;
	struct region *next_region;
	bool contains_descriptors;
} region_t;

typedef union descriptor {
	cache_t c;
	region_t r;
	union descriptor *next_free;
} descriptor_t;

typedef struct slab {
	cache_t *first_cache;		// linked list of caches
} slab_t;

struct mem_allocator {
	const slab_type_t *types;
	slab_t *slabs;

	descriptor_t *first_free_descriptor;
	region_t *all_regions;

	page_alloc_fun_t alloc_fun;
	page_free_fun_t free_fun;
};

// ============================================== //
// Helper functions for the manipulation of lists //
// ============================================== //

void add_free_descriptor(mem_allocator_t *a, descriptor_t *c) {
	c->next_free = a->first_free_descriptor;
	a->first_free_descriptor = c;
}

descriptor_t *take_descriptor(mem_allocator_t *a) {
	if (a->first_free_descriptor == 0) {
		void* p = a->alloc_fun(PAGE_SIZE);
		if (p == 0) return 0;

		const void* end = p + PAGE_SIZE;
		for (descriptor_t *i = (descriptor_t*)p; i + 1 <= (descriptor_t*)end; i++) {
			add_free_descriptor(a, i);
		}

		// register the descriptor region
		descriptor_t *dd = a->first_free_descriptor;
		ASSERT(dd != 0);
		a->first_free_descriptor = dd->next_free;

		region_t *drd = &dd->r;
		drd->region_addr = p;
		drd->region_size = PAGE_SIZE;
		drd->contains_descriptors = true;
		drd->next_region = a->all_regions;
		a->all_regions = drd;
	}

	descriptor_t *x = a->first_free_descriptor;
	ASSERT(x != 0);
	a->first_free_descriptor = x->next_free;
	return x;
}

// ============================== //
// The actual allocator functions //
// ============================== //

mem_allocator_t* create_slab_allocator(const slab_type_t *types, page_alloc_fun_t af, page_free_fun_t ff) {
	union {
		void* addr;
		mem_allocator_t *a;
		slab_t *s;
		descriptor_t *d;
	} ptr;

	ptr.addr = af(PAGE_SIZE);
	if (ptr.addr == 0) return 0;	// could not allocate
	const void* end_addr = ptr.addr + PAGE_SIZE;

	mem_allocator_t *a = ptr.a;
	ptr.a++;

	a->all_regions = 0;
	a->alloc_fun = af;
	a->free_fun = ff;

	a->types = types;
	a->slabs = ptr.s;
	for (const slab_type_t *t = types; t->obj_size != 0; t++) {
		ASSERT(t->obj_size >= sizeof(object_t));
		ptr.s->first_cache = 0;
		ptr.s++;
	}

	a->first_free_descriptor = 0;
	while (ptr.d + 1 <= (descriptor_t*)end_addr) {
		add_free_descriptor(a, ptr.d);
		ptr.d++;
	}

	return a;
}

void stack_and_destroy_regions(page_free_fun_t ff, region_t *r) {
	if (r == 0) return;
	void* addr = r->region_addr;
	ASSERT(r != r->next_region);
	stack_and_destroy_regions(ff, r->next_region);
	ff(addr);
}
void destroy_slab_allocator(mem_allocator_t *a) {
	for (int i = 0; a->types[i].obj_size != 0; i++) {
		for (cache_t *c = a->slabs[i].first_cache; c != 0; c = c->next_cache) {
			a->free_fun(c->region_addr);
		}
	}
	region_t *dr = 0;
	region_t *i = a->all_regions;
	while (i != 0) {
		region_t *r = i;
		i = r->next_region;
		if (r->contains_descriptors) {
			r->next_region = dr;
			dr = r;
		} else {
			a->free_fun(r->region_addr);
		}
	}
	stack_and_destroy_regions(a->free_fun, dr);
	a->free_fun(a);
}

void* slab_alloc(mem_allocator_t* a, size_t sz) {
	for (int i = 0; a->types[i].obj_size != 0; i++) {
		const size_t obj_size = a->types[i].obj_size;

		if (sz > obj_size) continue;

		// find a cache with free space
		cache_t *fc = a->slabs[i].first_cache;
		while (fc != 0 && fc->n_free_objs == 0) {
			// make sure n_free == 0 iff no object in the free stack
			ASSERT((fc->first_free_obj == 0) == (fc->n_free_objs == 0));
			fc = fc->next_cache;
		}
		// if none found, try to allocate a new one
		if (fc == 0) {
			descriptor_t *fcd = take_descriptor(a);
			if (fcd == 0) return 0;

			fc = &fcd->c;
			ASSERT((descriptor_t*)fc == fcd);

			const size_t cache_size = a->types[i].pages_per_cache * PAGE_SIZE;
			fc->region_addr = a->alloc_fun(cache_size);
			if (fc->region_addr == 0) {
				add_free_descriptor(a, fcd);
				return 0;
			}
			/*dbg_printf("New cache 0x%p\n", fc->region_addr);*/

			fc->n_free_objs = 0;
			fc->first_free_obj = 0;
			for (void* p = fc->region_addr; p + obj_size <= fc->region_addr + cache_size; p += obj_size) {
				object_t *x = (object_t*)p;
				x->next = fc->first_free_obj;
				fc->first_free_obj = x;
				fc->n_free_objs++;
			}
			ASSERT(fc->n_free_objs == cache_size / obj_size);

			fc->next_cache = a->slabs[i].first_cache;
			a->slabs[i].first_cache = fc;
		}
		// allocate on fc
		ASSERT(fc != 0);
		ASSERT(fc->n_free_objs > 0);
		ASSERT(fc->first_free_obj != 0);

		object_t *x = fc->first_free_obj;
		/*dbg_printf("Alloc 0x%p\n", x);*/
		fc->first_free_obj = x->next;
		fc->n_free_objs--;

		ASSERT((fc->n_free_objs == 0) == (fc->first_free_obj == 0));

		// TODO : if fc is full, put it at the end
		return x;
	}

	// otherwise directly allocate using a->alloc_fun
	descriptor_t *rd = take_descriptor(a);
	if (rd == 0) return 0;
	region_t *r = &rd->r;
	ASSERT((descriptor_t*)r == rd);

	r->region_addr = a->alloc_fun(sz);
	if (r->region_addr == 0) {
		add_free_descriptor(a, rd);
		return 0;
	} else {
		r->region_size = sz;
		r->contains_descriptors = false;

		r->next_region = a->all_regions;
		a->all_regions = r;

		return (void*)r->region_addr;
	}
}

void slab_free(mem_allocator_t* a, void* addr) {
	for (int i = 0; a->types[i].obj_size != 0; i++) {
		size_t region_size = PAGE_SIZE * a->types[i].pages_per_cache;
		for (cache_t *r = a->slabs[i].first_cache; r != 0; r = r->next_cache) {
			if (addr >= r->region_addr && addr < r->region_addr + region_size) {
				ASSERT((addr - r->region_addr) % a->types[i].obj_size == 0);

				object_t *o = (object_t*)addr;

				// check the object is not already on the free list (double-free error)
				for (object_t *i = r->first_free_obj; i != 0; i = i->next) {
					ASSERT((void*)i >= r->region_addr && (void*)i < r->region_addr + region_size);
					ASSERT(o != i);
				}

				/*dbg_printf("Put back 0x%p in 0x%p\n", o, r->region_addr);*/

				o->next = r->first_free_obj;
				r->first_free_obj = o;
				r->n_free_objs++;

				if (r->n_free_objs == region_size / a->types[i].obj_size) {
					// region is completely unused, free it.
					if (a->slabs[i].first_cache == r) {
						a->slabs[i].first_cache = r->next_cache;
					} else {
						for (cache_t *it = a->slabs[i].first_cache; it->next_cache != 0; it = it->next_cache) {
							if (it->next_cache == r) {
								it->next_cache = r->next_cache;
								break;
							}
						}
					}
					a->free_fun(r->region_addr);
					add_free_descriptor(a, (descriptor_t*)r);
				}
				return;
			}
		}
	}

	// otherwise the block was directly allocated : look for it in regions.
	ASSERT(a->all_regions != 0);

	if (a->all_regions->region_addr == addr) {
		a->free_fun(addr);	// found it, free it

		region_t *r = a->all_regions;
		a->all_regions = r->next_region;
		add_free_descriptor(a, (descriptor_t*)r);
	} else {
		for (region_t *i = a->all_regions; i->next_region != 0; i = i->next_region) {
			if (i->next_region->region_addr == addr) {
				a->free_fun(addr);	// found it, free it

				region_t *r = i->next_region;
				ASSERT(!r->contains_descriptors);
				i->next_region = r->next_region;
				add_free_descriptor(a, (descriptor_t*)r);
				return;
			}
		}
		ASSERT(false);
	}
}

/* vim: set ts=4 sw=4 tw=0 noet :*/

