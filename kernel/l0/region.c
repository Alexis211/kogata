#include <region.h>
#include <dbglog.h>
#include <frame.h>

typedef union region_descriptor {
	struct {
		union region_descriptor *next;
	} unused_descriptor;
	struct {
		size_t addr, size;
		union region_descriptor *next_by_size, *first_bigger;
		union region_descriptor *next_by_addr;
	} free;
	struct {
		region_info_t i;
		union region_descriptor *next_by_addr;
	} used;
} descriptor_t;

#define N_RESERVE_DESCRIPTORS 2		// always keep at least 3 unused descriptors

#define N_BASE_DESCRIPTORS 12		// pre-allocate memory for 12 descriptors
static descriptor_t base_descriptors[N_BASE_DESCRIPTORS];

static descriptor_t *first_unused_descriptor; 
uint32_t n_unused_descriptors;
static descriptor_t *first_free_region_by_addr, *first_free_region_by_size;
static descriptor_t *first_used_region;

// ========================================================= //
// HELPER FUNCTIONS FOR THE MANIPULATION OF THE REGION LISTS //
// ========================================================= //

static void add_unused_descriptor(descriptor_t *d) {
	n_unused_descriptors++;
	d->unused_descriptor.next = first_unused_descriptor;
	first_unused_descriptor = d;
}

static descriptor_t *get_unused_descriptor() {
	descriptor_t *r = first_unused_descriptor;
	if (r != 0) {
		first_unused_descriptor = r->unused_descriptor.next;
		n_unused_descriptors--;
	}
	return r;
}

static void remove_free_region(descriptor_t *d) {
	if (first_free_region_by_size == d) {
		first_free_region_by_size = d->free.next_by_size;
	} else {
		for (descriptor_t *i = first_free_region_by_size; i != 0; i = i->free.next_by_size) {
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

static void add_free_region(descriptor_t *d) {
	/*dbg_printf("Add free region 0x%p - 0x%p\n", d->free.addr, d->free.size + d->free.addr);*/
	// Find position of region in address-ordered list
	// Possibly concatenate free region
	descriptor_t *i = first_free_region_by_addr;
	if (i == 0) {
		ASSERT(first_free_region_by_size == 0);
		first_free_region_by_addr = first_free_region_by_size = d;
		d->free.next_by_size = d->free.first_bigger = d->free.next_by_addr = 0;
		return;
	} else if (d->free.addr + d->free.size == i->free.addr) {
		// concatenate d . i
		remove_free_region(i);
		d->free.size += i->free.size;
		add_unused_descriptor(i);
		add_free_region(d);
		return;
	} else if (i->free.addr > d->free.addr) {
		// insert before i
		d->free.next_by_addr = i;
		first_free_region_by_addr = d;
	} else {
		while (i != 0) {
			ASSERT(d->free.addr > i->free.addr);
			if (i->free.addr + i->free.size == d->free.addr) {
				// concatenate i . d
				remove_free_region(i);
				i->free.size += d->free.size;
				add_unused_descriptor(d);
				add_free_region(i);
				return;
			} else if (i->free.next_by_addr == 0) {
				i->free.next_by_addr = d;
				d->free.next_by_addr = 0;
				break;
			} else if (d->free.addr + d->free.size == i->free.next_by_addr->free.addr) {
				// concatenate d . i->next_by_addr
				descriptor_t *j = i->free.next_by_addr;
				remove_free_region(j);
				d->free.size += j->free.size;
				add_unused_descriptor(j);
				add_free_region(d);
				return;
			} else if (i->free.next_by_addr->free.addr > d->free.addr) {
				// insert between i and i->next_by_addr
				d->free.next_by_addr = i->free.next_by_addr;
				i->free.next_by_addr = d;
				break;
			} else {
				// continue
				i = i->free.next_by_addr;
			}
		}
	}
	// Now add it in size-ordered list
	i = first_free_region_by_size;
	ASSERT(i != 0);
	if (d->free.size <= i->free.size) {
		d->free.next_by_size = i;
		d->free.first_bigger = (i->free.size > d->free.size ? i : i->free.first_bigger);
		first_free_region_by_size = d;
	} else {
		while (i != 0) {
			ASSERT(d->free.size > i->free.size);
			if (i->free.next_by_size == 0) {
				i->free.next_by_size = d;
				if (d->free.size > i->free.size) i->free.first_bigger = d;
				d->free.next_by_size = 0;
				d->free.first_bigger = 0;
				break;
			} else if (i->free.next_by_size->free.size >= d->free.size) {
				i->free.next_by_size = d;
				if (d->free.size > i->free.size) i->free.first_bigger = d;
				d->free.next_by_size = i->free.next_by_size;
				d->free.first_bigger =
					(i->free.next_by_size->free.size > d->free.size
						? i->free.next_by_size
						: i->free.next_by_size->free.first_bigger);
				break;
			} else {
				// continue
				i = i->free.next_by_size;
			}
		}
	}
}

static descriptor_t *find_used_region(size_t addr) {
	for (descriptor_t *i = first_used_region; i != 0; i = i->used.next_by_addr) {
		if (addr >= i->used.i.addr && addr < i->used.i.addr + i->used.i.size) return i;
		if (i->used.i.addr > addr) break;
	}
	return 0;
}

static void add_used_region(descriptor_t *d) {
	descriptor_t *i = first_used_region;
	ASSERT(i->used.i.addr < d->used.i.addr);	// first region by address is never free

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

static void remove_used_region(descriptor_t *d) {
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

void region_allocator_init(void* kernel_data_end) {
	descriptor_t *u0 = &base_descriptors[0];
	u0->used.i.addr = K_HIGHHALF_ADDR;
	u0->used.i.size = PAGE_ALIGN_UP(kernel_data_end) - K_HIGHHALF_ADDR;
	u0->used.i.type = REGION_T_KERNEL_BASE;
	u0->used.i.pf = 0;
	u0->used.next_by_addr = 0;
	first_used_region = u0;

	descriptor_t *f0 = &base_descriptors[1];
	f0->free.addr = PAGE_ALIGN_UP(kernel_data_end);
	f0->free.size = (LAST_KERNEL_ADDR-f0->free.addr);
	f0->free.next_by_size = 0;
	f0->free.first_bigger = 0;
	first_free_region_by_size = first_free_region_by_addr = f0;

	n_unused_descriptors = 0;
	first_unused_descriptor = 0;
	for (int i = 2; i < N_BASE_DESCRIPTORS; i++) {
		add_unused_descriptor(&base_descriptors[i]);
	}
}

static size_t region_alloc_inner(size_t size, uint32_t type, page_fault_handler_t pf, bool use_reserve) {
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
				x = get_unused_descriptor();
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

			size_t addr = i->free.addr;
			i->used.i.addr = addr;
			i->used.i.size = size;
			i->used.i.type = type;
			i->used.i.pf = pf;
			add_used_region(i);

			return addr;
		}
	}
	return 0;	//No big enough block found
}

size_t region_alloc(size_t size, uint32_t type, page_fault_handler_t pf) {
	if (n_unused_descriptors <= N_RESERVE_DESCRIPTORS) {
		uint32_t frame = frame_alloc(1);
		if (frame == 0) return 0;

		size_t descriptor_region = region_alloc_inner(PAGE_SIZE, REGION_T_DESCRIPTORS, 0, true);
		ASSERT(descriptor_region != 0);

		int error = pd_map_page(descriptor_region, frame, 1);
		if (error) {
			// this can happen if we weren't able to allocate a frame for
			// a new pagetable
			frame_free(frame, 1);
			return 0;
		}
		
		for (descriptor_t *d = (descriptor_t*)descriptor_region;
				(size_t)(d+1) <= (descriptor_region + PAGE_SIZE);
				d++) {
			add_unused_descriptor(d);
		}
	}
	return region_alloc_inner(size, type, pf, false);
}

region_info_t *find_region(size_t addr) {
	descriptor_t *d = find_used_region(addr);
	if (d == 0) return 0;
	return &d->used.i;
}

void region_free(size_t addr) {
	descriptor_t *d = find_used_region(addr);
	if (d == 0) return;

	region_info_t i = d->used.i;

	remove_used_region(d);
	d->free.addr = i.addr;
	d->free.size = i.size;
	add_free_region(d);
}

void dbg_print_region_stats() {
	dbg_printf("/ Free kernel regions, by address:\n");
	for (descriptor_t *d = first_free_region_by_addr; d != 0; d = d->free.next_by_addr) {
		dbg_printf("| 0x%p - 0x%p\n", d->free.addr, d->free.addr + d->free.size);
		ASSERT(d != d->free.next_by_addr);
	}
	dbg_printf("- Free kernel regions, by size:\n");
	for (descriptor_t *d = first_free_region_by_size; d != 0; d = d->free.next_by_size) {
		dbg_printf("| 0x%p - 0x%p\n", d->free.addr, d->free.addr + d->free.size);
		ASSERT(d != d->free.next_by_size);
	}
	dbg_printf("- Used kernel regions:\n");
	for (descriptor_t *d = first_used_region; d != 0; d = d->used.next_by_addr) {
		dbg_printf("| 0x%p - 0x%p", d->used.i.addr, d->used.i.addr + d->used.i.size);
		if (d->used.i.type & REGION_T_KERNEL_BASE)	dbg_printf("  Kernel code & base data");
		if (d->used.i.type & REGION_T_DESCRIPTORS)	dbg_printf("  Region descriptors");
		if (d->used.i.type & REGION_T_PAGETABLE)	dbg_printf("  Mapped PD/PT");
		if (d->used.i.type & REGION_T_CORE_HEAP) 	dbg_printf("  Core heap");
		if (d->used.i.type & REGION_T_PROC_HEAP)	dbg_printf("  Kernel process heap");
		if (d->used.i.type & REGION_T_CACHE)		dbg_printf("  Cache");
		if (d->used.i.type & REGION_T_HW)			dbg_printf("  Hardware");
		dbg_printf("\n");
		ASSERT(d != d->used.next_by_addr);
	}
	dbg_printf("\\\n");
}
