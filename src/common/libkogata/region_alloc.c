#include <kogata/region_alloc.h>
#include <kogata/debug.h>
#include <kogata/mutex.h>

// we cannot include sys...

#define PAGE_SIZE		0x1000
#define PAGE_MASK		0xFFFFF000
#define PAGE_ALIGNED(x)     ((((size_t)(x)) & (~PAGE_MASK)) == 0)
#define PAGE_ALIGN_DOWN(x)	(((size_t)(x)) & PAGE_MASK) 
#define PAGE_ALIGN_UP(x)  	((((size_t)(x))&(~PAGE_MASK)) == 0 ? ((size_t)x) : (((size_t)x) & PAGE_MASK) + PAGE_SIZE)

typedef union region_descriptor {
	struct {
		union region_descriptor *next;
	} unused_descriptor;
	struct {
		void* addr;
		size_t size;
		union region_descriptor *next_by_size;
		union region_descriptor *first_bigger;
		union region_descriptor *next_by_addr;
	} free;
	struct {
		region_info_t i;
		union region_descriptor *next_by_addr;
	} used;
} descriptor_t;

#define N_RESERVE_DESCRIPTORS 2		// always keep at least 2 unused descriptors

#define N_BASE_DESCRIPTORS 12		// pre-allocate memory for 12 descriptors
static descriptor_t base_descriptors[N_BASE_DESCRIPTORS];

static descriptor_t *first_unused_descriptor; 
uint32_t n_unused_descriptors;
static descriptor_t *first_free_region_by_addr, *first_free_region_by_size;
static descriptor_t *first_used_region;	// by address

static map_page_fun_t region_alloc_map_page_fun;

STATIC_MUTEX(ra_mutex);	// region allocator mutex

// ========================================================= //
// HELPER FUNCTIONS FOR THE MANIPULATION OF THE REGION LISTS //
// ========================================================= //

void add_unused_region_descriptor(descriptor_t *d) {
	n_unused_descriptors++;
	d->unused_descriptor.next = first_unused_descriptor;
	first_unused_descriptor = d;
}

descriptor_t *get_unused_region_descriptor() {
	descriptor_t *r = first_unused_descriptor;
	ASSERT(r != 0);

	first_unused_descriptor = r->unused_descriptor.next;
	n_unused_descriptors--;
	return r;
}

void remove_free_region(descriptor_t *d) {
	if (first_free_region_by_size == d) {
		first_free_region_by_size = d->free.next_by_size;
	} else {
		for (descriptor_t *i = first_free_region_by_size; i != 0; i = i->free.next_by_size) {
			if (i->free.first_bigger == d) {
				i->free.first_bigger = d->free.next_by_size;
			}
			if (i->free.next_by_size == d) {
				i->free.next_by_size = d->free.next_by_size;
				break;
			}
		}
	}
	if (first_free_region_by_addr == d) {
		first_free_region_by_addr = d->free.next_by_addr;
	} else {
		for (descriptor_t *i = first_free_region_by_addr; i != 0; i = i->free.next_by_addr) {
			if (i->free.next_by_addr == d) {
				i->free.next_by_addr = d->free.next_by_addr;
				break;
			}
		}
	}
}

void _region_alloc_rec_check_first_bigger() {
	descriptor_t *it = first_free_region_by_size;

	descriptor_t *supposed_first_bigger = it->free.first_bigger;
	size_t sz = it->free.size;

	while (it != NULL) {
		it = it->free.next_by_size;
		if (it == NULL) {
			ASSERT(supposed_first_bigger == NULL);
		} else if (it->free.size == sz) {
			ASSERT(it->free.first_bigger == supposed_first_bigger);
		} else if (it->free.size > sz) {
			ASSERT(it == supposed_first_bigger);
			supposed_first_bigger = it->free.first_bigger;
			sz = it->free.size;
		} else {
			ASSERT(false);
		}
	}
}

void add_free_region(descriptor_t *d) {
	// dbg_printf("Add free region 0x%p - 0x%p\n", d->free.addr, d->free.size + d->free.addr);

	for (descriptor_t *it = first_free_region_by_addr; it != NULL; it = it->free.next_by_addr) {
		if (it->free.addr + it->free.size == d->free.addr) {
			// ----- PREV-CONCATENATE -----
			descriptor_t *prev_free = it;
			// dbg_printf("Prev-concatenate 0x%p - 0x%p\n", prev_free->free.addr, prev_free->free.size + prev_free->free.addr);

			remove_free_region(prev_free);
			prev_free->free.size += d->free.size;
			add_unused_region_descriptor(d);
			add_free_region(prev_free);
			return;
		} else if (it->free.addr == d->free.addr + d->free.size) {
			// ----- NEXT-CONCATENATE -----
			descriptor_t *next_free = it;
			// dbg_printf("Next-concatenate 0x%p - 0x%p\n", next_free->free.addr, next_free->free.size + next_free->free.addr);

			remove_free_region(next_free);
			d->free.size += next_free->free.size;
			add_unused_region_descriptor(next_free);
			add_free_region(d);
			return;
		} else if (it->free.addr > d->free.addr) {
			break;
		}
	}

	// We must insert this region in the two lists

	if (first_free_region_by_addr == NULL) {
		ASSERT(first_free_region_by_size == NULL);
		d->free.next_by_addr = NULL;
		first_free_region_by_addr = d;

		d->free.next_by_size = NULL;
		d->free.first_bigger = NULL;
		first_free_region_by_size = d;
		return;
	}

	ASSERT(first_free_region_by_addr != NULL && first_free_region_by_size != NULL);

	// First, the by-address list
	if (first_free_region_by_addr->free.addr > d->free.addr) {
		d->free.next_by_addr = first_free_region_by_addr;
		first_free_region_by_addr = d;
	} else {
		for (descriptor_t *it = first_free_region_by_addr; it != NULL; it = it->free.next_by_addr) {
			if (it->free.next_by_addr == NULL || it->free.next_by_addr->free.addr > d->free.addr) {
				d->free.next_by_addr = it->free.next_by_addr;;
				it->free.next_by_addr = d;
				break;
			}
		}
	}
	// Now, the by-size list
	// If we have many of this size, insert as the first
	if (first_free_region_by_size->free.size >= d->free.size) {
		d->free.next_by_size = first_free_region_by_size;
		d->free.first_bigger = (first_free_region_by_size->free.size > d->free.size
									? first_free_region_by_size : first_free_region_by_size->free.first_bigger);
		first_free_region_by_size = d;
	} else {
		for (descriptor_t *it = first_free_region_by_size; it != NULL; it = it->free.next_by_size) {
			ASSERT(it->free.size < d->free.size);
			// Maybee we are the new first_bigger of it ?
			if (it->free.first_bigger == NULL || it->free.first_bigger->free.size >= d->free.size) {
				it->free.first_bigger = d;
			}
			// Maybee insert after here ?
			if (it->free.next_by_size == NULL) {
				d->free.next_by_size = NULL;
				it->free.next_by_size = d;
				d->free.first_bigger = NULL;
				break;
			} else if (it->free.next_by_size->free.size >= d->free.size) {
				descriptor_t *next = it->free.next_by_size;
				d->free.next_by_size = next;
				it->free.next_by_size = d;
				d->free.first_bigger = (next->free.size > d->free.size ? next : next->free.first_bigger);
				break;
			}
		}
	}

	_region_alloc_rec_check_first_bigger();
}

descriptor_t *find_used_region(void* addr) {
	for (descriptor_t *i = first_used_region; i != 0; i = i->used.next_by_addr) {
		if (addr >= i->used.i.addr && addr < i->used.i.addr + i->used.i.size) return i;
		if (i->used.i.addr > addr) break;
	}
	return 0;
}

void add_used_region(descriptor_t *d) {
	if (first_used_region == 0 || d->used.i.addr < first_used_region->used.i.addr) {
		d->used.next_by_addr = first_used_region;
		first_used_region = d;
	} else {
		descriptor_t *i = first_used_region;
		ASSERT(i->used.i.addr < d->used.i.addr);

		while (i != 0) {
			ASSERT(i->used.i.addr < d->used.i.addr);
			if (i->used.next_by_addr == 0 || i->used.next_by_addr->used.i.addr > d->used.i.addr) {
				d->used.next_by_addr = i->used.next_by_addr;
				i->used.next_by_addr = d;
				return;
			} else {
				i = i->used.next_by_addr;
			}
		}
		ASSERT(false);
	}
}

void remove_used_region(descriptor_t *d) {
	if (first_used_region == d) {
		first_used_region = d->used.next_by_addr;
	} else {
		for (descriptor_t *i = first_used_region; i != 0; i = i->used.next_by_addr) {
			if (i->used.i.addr > d->used.i.addr) break;
			if (i->used.next_by_addr == d) {
				i->used.next_by_addr = d->used.next_by_addr;
				break;
			}
		}
	}
}

// =============== //
// THE ACTUAL CODE //
// =============== //

void region_allocator_init(void* begin, void* rsvd_end, void* end, map_page_fun_t map_page_fun) {
	n_unused_descriptors = 0;
	first_unused_descriptor = 0;
	for (int i = 0; i < N_BASE_DESCRIPTORS; i++) {
		add_unused_region_descriptor(&base_descriptors[i]);
	}

	ASSERT(PAGE_ALIGNED(begin));
	ASSERT(PAGE_ALIGNED(rsvd_end));
	ASSERT(PAGE_ALIGNED(end));

	descriptor_t *f0 = get_unused_region_descriptor();
	f0->free.addr = rsvd_end;
	f0->free.size = ((void*)end - rsvd_end);
	first_free_region_by_size = first_free_region_by_addr = NULL;
	add_free_region(f0);

	if (rsvd_end > begin) {
		descriptor_t *u0 = get_unused_region_descriptor();
		u0->used.i.addr = begin;
		u0->used.i.size = rsvd_end - begin;
		u0->used.i.type = "Reserved";
		u0->used.next_by_addr = 0;
		first_used_region = u0;
	} else {
		first_used_region = 0;
	}

	region_alloc_map_page_fun = map_page_fun;
}

void region_free_inner(void* addr) {
	descriptor_t *d = find_used_region(addr);
	if (d == 0) return;

	region_info_t i = d->used.i;

	remove_used_region(d);
	d->free.addr = i.addr;
	d->free.size = i.size;
	add_free_region(d);
}
void region_free(void* addr) {
	mutex_lock(&ra_mutex);
	region_free_inner(addr);
	mutex_unlock(&ra_mutex);
}

void* region_alloc_inner(size_t size, char* type, bool use_reserve) {
	size = PAGE_ALIGN_UP(size);

	for (descriptor_t *i = first_free_region_by_size; i != 0; i = i->free.first_bigger) {
		if (i->free.size >= size) {
			// region i is the one we want to allocate in
			descriptor_t *x = 0;
			if (i->free.size > size) {
				if (n_unused_descriptors <= N_RESERVE_DESCRIPTORS && !use_reserve) {
					return 0;
				}

				// this assert basically means that the allocation function
				// is called less than N_RESERVE_DESCRIPTORS times with
				// the use_reserve flag before more descriptors
				// are allocated.
				x = get_unused_region_descriptor();
				ASSERT(x != 0);

				x->free.size = i->free.size - size;
				if (size >= 0x4000) {
					x->free.addr = i->free.addr + size;
				} else {
					x->free.addr = i->free.addr;
					i->free.addr += x->free.size;
				}
			}
			// do the allocation
			remove_free_region(i);
			if (x != 0) add_free_region(x);

			void* addr = i->free.addr;
			i->used.i.addr = addr;
			i->used.i.size = size;
			i->used.i.type = type;
			add_used_region(i);

			return addr;
		}
	}
	return 0;	//No big enough block found
}

void* region_alloc(size_t size, char* type) {
	void* result = 0;
	mutex_lock(&ra_mutex);

	if (n_unused_descriptors <= N_RESERVE_DESCRIPTORS) {
		void* descriptor_region = region_alloc_inner(PAGE_SIZE, "Region descriptors", true);
		ASSERT(descriptor_region != 0);

		bool map_ok = region_alloc_map_page_fun(descriptor_region);
		if (!map_ok) {
			// this can happen if we weren't able to allocate a frame for
			// a new pagetable
			region_free_inner(descriptor_region);
			goto try_anyway;
		}
		
		for (descriptor_t *d = (descriptor_t*)descriptor_region;
				(void*)(d+1) <= (descriptor_region + PAGE_SIZE);
				d++) {
			add_unused_region_descriptor(d);
		}
	}
	try_anyway:
	// even if we don't have enough unused descriptors, we might find
	// a free region that has exactly the right size and therefore
	// does not require splitting, so we try the allocation in all cases
	result = region_alloc_inner(size, type, false);

	mutex_unlock(&ra_mutex);
	return result;
}

region_info_t *find_region(void* addr) {
	region_info_t *r = 0;
	mutex_lock(&ra_mutex);

	descriptor_t *d = find_used_region(addr);
	if (d != 0) r = &d->used.i;

	mutex_unlock(&ra_mutex);
	return r;
}


// =========================== //
// DEBUG LOG PRINTING FUNCTION //
// =========================== //

void dbg_print_region_info() {
	mutex_lock(&ra_mutex);

	dbg_printf("/ Free VM regions, by address:\n");
	for (descriptor_t *d = first_free_region_by_addr; d != 0; d = d->free.next_by_addr) {
		dbg_printf("| 0x%p - 0x%p\n", d->free.addr, d->free.addr + d->free.size);
		ASSERT(d != d->free.next_by_addr);
	}
	dbg_printf("- Free VM regions, by size:\n");
	for (descriptor_t *d = first_free_region_by_size; d != 0; d = d->free.next_by_size) {
		dbg_printf("| 0x%p - 0x%p ", d->free.addr, d->free.addr + d->free.size);
		dbg_printf("(0x%p, next in size: 0x%p)\n", d->free.size,
			(d->free.first_bigger == 0 ? 0 : d->free.first_bigger->free.addr));
		ASSERT(d != d->free.next_by_size);
	}
	dbg_printf("- Used VM regions:\n");
	for (descriptor_t *d = first_used_region; d != 0; d = d->used.next_by_addr) {
		dbg_printf("| 0x%p - 0x%p  %s\n", d->used.i.addr, d->used.i.addr + d->used.i.size, d->used.i.type);
		ASSERT(d != d->used.next_by_addr);
	}

	int nfreerd = 0;
	for (descriptor_t *i = first_unused_descriptor; i != 0; i = i->unused_descriptor.next)
		nfreerd++;

	dbg_printf("\\ Free region descriptors: %d (%d)\n", nfreerd, n_unused_descriptors);

	mutex_unlock(&ra_mutex);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
