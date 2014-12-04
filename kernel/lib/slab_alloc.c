#include <slab_alloc.h>

typedef struct object {
	struct object *next;
} object_t;

typedef struct cache {
	struct {	// pack this so that it takes less space...
		uint32_t is_a_cache		: 1;		// if not, this is a region allocated alone
		uint32_t n_free_objs	: 31;
	};

	union {
		struct {	// when this is a cache
			object_t* first_free_obj;
			struct cache *next_cache;
		} c;
		struct {	// when this is a region allocated alone
			size_t region_size;
		} sr;
	};

	size_t region_addr;
	struct cache *next_region;
} cache_t;

typedef struct slab {
	cache_t *first_cache;		// linked list of caches
} slab_t;

struct mem_allocator {
	const slab_type_t *types;
	slab_t *slabs;
	cache_t *first_free_region_descriptor;
	cache_t *all_regions;

	page_alloc_fun_t alloc_fun;
	page_free_fun_t free_fun;
};

// ============================================== //
// Helper functions for the manipulation of lists //
// ============================================== //

void add_free_region_descriptor(mem_allocator_t *a, cache_t *c) {
	c->next_region = a->first_free_region_descriptor;
	a->first_free_region_descriptor = c;
}

cache_t *take_region_descriptor(mem_allocator_t *a) {
	if (a->first_free_region_descriptor == 0) {
		// TODO : allocate more descriptors	(not complicated)
		return 0;
	}
	cache_t *x = a->first_free_region_descriptor;
	a->first_free_region_descriptor = x->next_region;
	return x;
}

// ============================== //
// The actual allocator functions //
// ============================== //

mem_allocator_t* create_slab_allocator(const slab_type_t *types, page_alloc_fun_t af, page_free_fun_t ff) {
	union {
		size_t addr;
		mem_allocator_t *a;
		slab_t *s;
		cache_t *c;
	} ptr;

	ptr.addr = (size_t)af(PAGE_SIZE);
	if (ptr.addr == 0) return 0;	// could not allocate
	size_t end_addr = ptr.addr + PAGE_SIZE;

	mem_allocator_t *a = ptr.a;
	ptr.a++;

	a->all_regions = 0;
	a->alloc_fun = af;
	a->free_fun = ff;

	a->types = types;
	a->slabs = ptr.s;
	for (const slab_type_t *t = types; t->obj_size != 0; t++) {
		ptr.s->first_cache = 0;
		ptr.s++;
	}

	a->first_free_region_descriptor = 0;
	while ((size_t)(ptr.c + 1) <= end_addr) {
		add_free_region_descriptor(a, ptr.c);
		ptr.c++;
	}

	return a;
}

static void stack_and_destroy_regions(page_free_fun_t ff, cache_t *r) {
	if (r == 0) return;
	size_t addr = r->region_addr;
	stack_and_destroy_regions(ff, r->next_region);
	ff((void*)addr);
}
void destroy_slab_allocator(mem_allocator_t *a) {
	stack_and_destroy_regions(a->free_fun, a->all_regions);
	a->free_fun(a);
}

void* slab_alloc(mem_allocator_t* a, const size_t sz) {
	for (int i = 0; a->types[i].obj_size != 0; i++) {
		const size_t obj_size = a->types[i].obj_size;
		if (sz <= obj_size) {
			// find a cache with free space
			cache_t *fc = a->slabs[i].first_cache;
			while (fc != 0 && fc->n_free_objs == 0) {
				ASSERT(fc->c.first_free_obj == 0); // make sure n_free == 0 iff no object in the free stack
				fc = fc->c.next_cache;
			}
			// if none found, try to allocate a new one
			if (fc == 0) {
				fc = take_region_descriptor(a);
				if (fc == 0) return 0;

				const size_t cache_size = a->types[i].pages_per_cache * PAGE_SIZE;
				fc->region_addr = (size_t)a->alloc_fun(cache_size);
				if (fc->region_addr == 0) {
					add_free_region_descriptor(a, fc);
					return 0;
				}

				fc->is_a_cache = 1;
				fc->n_free_objs = 0;
				fc->c.first_free_obj = 0;
				for (size_t i = fc->region_addr; i + obj_size <= fc->region_addr + cache_size; i += obj_size) {
					object_t *x = (object_t*)i;
					x->next = fc->c.first_free_obj;
					fc->c.first_free_obj = x;
					fc->n_free_objs++;
				}
				ASSERT(fc->n_free_objs == cache_size / obj_size);

				fc->next_region = a->all_regions;
				a->all_regions = fc;
				fc->c.next_cache = a->slabs[i].first_cache;
				a->slabs[i].first_cache = fc;
			}
			// allocate on fc
			ASSERT(fc != 0 && fc->n_free_objs > 0);
			object_t *x = fc->c.first_free_obj;
			fc->c.first_free_obj = x->next;
			fc->n_free_objs--;
			// TODO : if fc is full, put it at the end
			return (void*)x;
		}
	}

	// otherwise directly allocate using a->alloc_fun
	cache_t *r = take_region_descriptor(a);
	if (r == 0) return 0;

	r->region_addr = (size_t)a->alloc_fun(sz);
	if (r->region_addr == 0) {
		add_free_region_descriptor(a, r);
		return 0;
	} else {
		r->is_a_cache =  0;
		r->sr.region_size = sz;

		r->next_region = a->all_regions;
		a->all_regions = r;

		return (void*)r->region_addr;
	}
}

void slab_free(mem_allocator_t* a, const void* ptr) {
	const size_t addr = (size_t)ptr;

	for (int i = 0; a->types[i].obj_size != 0; i++) {
		size_t region_size = PAGE_SIZE * a->types[i].pages_per_cache;
		for (cache_t *r = a->slabs[i].first_cache; r != 0; r = r->c.next_cache) {
			if (addr >= r->region_addr && addr < r->region_addr + region_size) {
				ASSERT((addr - r->region_addr) % a->types[i].obj_size == 0);
				ASSERT(r->is_a_cache);
				object_t *o = (object_t*)addr;
				o->next = r->c.first_free_obj;
				r->c.first_free_obj = o;
				r->n_free_objs++;
				// TODO : if cache is empty, free it
				return;
			}
		}
	}

	// otherwise the block was directly allocated : look for it in regions.
	a->free_fun(ptr);
	ASSERT(a->all_regions != 0);

	if (a->all_regions->region_addr == addr) {
		cache_t *r = a->all_regions;
		ASSERT(r->is_a_cache == 0);
		a->all_regions = r->next_region;
		add_free_region_descriptor(a, r);
	} else {
		for (cache_t *i = a->all_regions; i->next_region != 0; i = i->next_region) {
			if (i->next_region->region_addr == addr) {
				cache_t *r = i->next_region;
				ASSERT(r->is_a_cache == 0);
				i->next_region = r->next_region;
				add_free_region_descriptor(a, r);
				return;
			}
		}
		ASSERT(false);
	}
}
