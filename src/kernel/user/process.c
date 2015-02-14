#include <mutex.h>
#include <hashtbl.h>
#include <string.h>

#include <frame.h>
#include <process.h>

typedef struct user_region {
	void* addr;
	size_t size;

	int mode;

	fs_handle_t *file;		// null if not mmaped-file
	size_t file_offset;

	struct user_region *next;
} user_region_t;

static int next_pid = 1;

static void proc_user_exception(registers_t *regs);
static void proc_usermem_pf(void* proc, registers_t *regs, void* addr);

process_t *current_process() {
	if (current_thread) return current_thread->proc;
	return 0;
}

// ============================== //
// CREATING AND RUNNING PROCESSES //
// ============================== //

process_t *new_process(process_t *parent) {
	process_t *proc = (process_t*)malloc(sizeof(process_t));
	if (proc == 0) return 0;

	proc->filesystems = create_hashtbl(str_key_eq_fun, str_hash_fun, free_key);
	if (proc->filesystems == 0) {
		free(proc);
		return 0;
	}

	proc->pd = create_pagedir(proc_usermem_pf, proc);
	if (proc->pd == 0) {
		delete_hashtbl(proc->filesystems);
		free(proc);
		return 0;
	}

	proc->regions = 0;
	proc->thread = 0;
	proc->pid = (next_pid++);
	proc->parent = parent;

	return proc;
}

static void run_user_code(void* entry) {
	process_t *proc = current_thread->proc;
	ASSERT(proc != 0);

	switch_pagedir(proc->pd);

	void* esp = (void*)USERSTACK_ADDR + USERSTACK_SIZE;

	asm volatile("				\
			cli;				\
								\
			mov $0x23, %%ax;	\
			mov %%ax, %%ds;		\
			mov %%ax, %%es;		\
			mov %%ax, %%fs;		\
			mov %%ax, %%gs;		\
								\
			pushl $0x23;		\
			pushl %%ebx;		\
			pushf;				\
			pop %%eax;			\
			or $0x200, %%eax;	\
			pushl %%eax;		\
			pushl $0x1B;		\
			pushl %%ecx;		\
			iret				\
		"::"b"(esp),"c"(entry));
}
bool start_process(process_t *p, void* entry) {
	bool stack_ok = mmap(p, (void*)USERSTACK_ADDR, USERSTACK_SIZE, MM_READ | MM_WRITE);
	if (!stack_ok) return false;

	thread_t *th = new_thread(run_user_code, entry);
	if (th == 0) {
		munmap(p, (void*)USERSTACK_ADDR);
		return false;
	}

	th->proc = p;
	th->user_ex_handler = proc_user_exception;
	
	resume_thread(th, false);

	return true;
}


// ================================== //
// MANAGING FILESYSTEMS FOR PROCESSES //
// ================================== //

bool proc_add_fs(process_t *p, fs_t *fs, const char* name) {
	char *n = strdup(name);
	if (n == 0) return false;

	bool add_ok = hashtbl_add(p->filesystems, n, fs);
	if (!add_ok) {
		free(n);
		return false;
	}

	return true;
}

fs_t *proc_find_fs(process_t *p, const char* name) {
	return hashtbl_find(p->filesystems, name);
}

// ============================= //
// USER MEMORY REGION MANAGEMENT //
// ============================= //

static user_region_t *find_user_region(process_t *proc, const void* addr) {
	for (user_region_t *it = proc->regions; it != 0; it = it->next) {
		if (addr >= it->addr && addr < it->addr + it->size) return it;
	}
	return 0;
}

bool mmap(process_t *proc, void* addr, size_t size, int mode) {
	if (find_user_region(proc, addr) != 0) return false;

	if ((uint32_t)addr & (~PAGE_MASK)) return false;

	user_region_t *r = (user_region_t*)malloc(sizeof(user_region_t));
	if (r == 0) return false;

	r->addr = addr;
	r->size = PAGE_ALIGN_UP(size);

	if (r->addr >= (void*)K_HIGHHALF_ADDR || r->addr + r->size > (void*)K_HIGHHALF_ADDR || r->size == 0) {
		free(r);
		return false;
	}

	r->mode = mode;
	r->file = 0;
	r->file_offset = 0;

	r->next = proc->regions;
	proc->regions = r;

	return true;
}

bool mmap_file(process_t *proc, fs_handle_t *h, size_t offset, void* addr, size_t size, int mode) {
	if (find_user_region(proc, addr) != 0) return false;

	if ((uint32_t)addr & (~PAGE_MASK)) return false;

	user_region_t *r = (user_region_t*)malloc(sizeof(user_region_t));
	if (r == 0) return false;

	r->addr = addr;
	r->size = PAGE_ALIGN_UP(size);

	if (r->addr >= (void*)K_HIGHHALF_ADDR || r->addr + r->size > (void*)K_HIGHHALF_ADDR || r->size == 0) {
		free(r);
		return false;
	}

	ref_file(h);

	r->mode = mode;
	r->file = h;
	r->file_offset = offset;

	r->next = proc->regions;
	proc->regions = r;

	return true;
}

bool mchmap(process_t *proc, void* addr, int mode) {
	user_region_t *r = find_user_region(proc, addr);

	if (r == 0) return false;
	
	r->mode = mode;
	return true;
}

bool munmap(process_t *proc, void* addr) {
	user_region_t *r = find_user_region(proc, addr);
	if (r == 0) return false;

	if (proc->regions == r) {
		proc->regions = r->next;
	} else {
		for (user_region_t *it = proc->regions; it != 0; it = it->next) {
			if (it->next == r) {
				it->next = r->next;
				break;
			}
		}
	}

	// TODO : write modified pages back to file!
	// TODO : unmap mapped page for region...

	if (r->file != 0) unref_file(r->file);

	free(r);

	return true;
}

// =============================== //
// USER MEMORY PAGE FAULT HANDLERS //
// =============================== //

static void proc_user_exception(registers_t *regs) {
	dbg_printf("Usermode exception in user process : exiting.\n");
	dbg_dump_registers(regs);
	exit();
}
static void proc_usermem_pf(void* p, registers_t *regs, void* addr) {
	process_t *proc = (process_t*)p;

	user_region_t *r = find_user_region(proc, addr);
	if (r == 0) {
		dbg_printf("Segmentation fault in process %d (0x%p : not mapped) : exiting.\n", proc->pid, addr);
		exit();
	}

	bool wr = ((regs->err_code & PF_WRITE_BIT) != 0);
	if (wr && !(r->mode & MM_WRITE)) {
		dbg_printf("Segmentation fault in process %d (0x%p : not allowed to write) : exiting.\n", proc->pid, addr);
		exit();
	}

	bool pr = ((regs->err_code & PF_PRESENT_BIT) != 0);
	ASSERT(!pr);

	addr = (void*)((uint32_t)addr & PAGE_MASK);

	uint32_t frame;
	do {
		frame = frame_alloc(1);
		if (frame == 0) {
			dbg_printf("OOM for process %d ; yielding and waiting for someone to free some RAM.\n", proc->pid);
			yield();
		}
	} while (frame == 0);

	while(!pd_map_page(addr, frame, (r->mode & MM_WRITE) != 0)) {
		dbg_printf("OOM(2) for process %d ; yielding and waiting for someone to free some RAM.\n", proc->pid);
		yield();
	}

	memset(addr, 0, PAGE_SIZE);		// zero out

	if (r->file != 0) {
		file_read(r->file, addr - r->addr + r->file_offset, PAGE_SIZE, addr);
	}
}

void probe_for_read(const void* addr, size_t len) {
	process_t *proc = current_process();
	user_region_t *r = find_user_region(proc, addr);
	if (r == 0 || addr + len > r->addr + r->size || !(r->mode & MM_READ)) {
		dbg_printf("Access violation on read at 0x%p len 0x%p in process %d : exiting.\n",
			addr, len, proc->pid);
		exit();
	}
}

void probe_for_write(const void* addr, size_t len) {
	process_t *proc = current_process();
	user_region_t *r = find_user_region(proc, addr);
	if (r == 0 || addr + len > r->addr + r->size || !(r->mode & MM_WRITE)) {
		dbg_printf("Access violation on write at 0x%p len 0x%p in process %d : exiting.\n",
			addr, len, proc->pid);
		exit();
	}
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
