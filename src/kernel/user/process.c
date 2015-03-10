#include <mutex.h>
#include <hashtbl.h>
#include <string.h>

#include <frame.h>
#include <process.h>
#include <freemem.h>
#include <worker.h>

static int next_pid = 1;

void proc_user_exception(registers_t *regs);
void proc_usermem_pf(void* proc, registers_t *regs, void* addr);

process_t *current_process() {
	if (current_thread) return current_thread->proc;
	return 0;
}

typedef struct {
	proc_entry_t entry;
	void *sp;
} setup_data_t;

typedef struct {
	process_t *proc;
	int status, exit_code;
} exit_data_t;

// ============================== //
// CREATING AND RUNNING PROCESSES //
// ============================== //

process_t *new_process(process_t *parent) {
	process_t *proc = (process_t*)malloc(sizeof(process_t));
	if (proc == 0) goto error;

	proc->filesystems = create_hashtbl(str_key_eq_fun, str_hash_fun, free_key);
	if (proc->filesystems == 0) goto error;

	proc->files = create_hashtbl(id_key_eq_fun, id_hash_fun, 0);
	if (proc->files == 0) goto error;

	proc->regions_idx = create_btree(id_key_cmp_fun, 0);
	if (proc->regions_idx == 0) goto error;

	proc->pd = create_pagedir(proc_usermem_pf, proc);
	if (proc->pd == 0) goto error;

	proc->last_ran = 0;
	proc->regions = 0;
	proc->threads = 0;

	proc->parent = parent;
	proc->children = 0;
	proc->next_child = 0;

	proc->next_fd = 1;
	proc->pid = (next_pid++);
	proc->status = PS_LOADING;
	proc->exit_code = 0;

	proc->lock = MUTEX_UNLOCKED;

	if (parent != 0) {
		mutex_lock(&parent->lock);

		proc->next_child = parent->children;
		parent->children = proc;

		mutex_unlock(&parent->lock);
	}

	return proc;

error:
	if (proc && proc->regions_idx) delete_btree(proc->regions_idx);
	if (proc && proc->filesystems) delete_hashtbl(proc->filesystems);
	if (proc && proc->files) delete_hashtbl(proc->files);
	if (proc) free(proc);
	return 0;
}

static void run_user_code(void* param) {
	setup_data_t *d = (setup_data_t*)param;

	void* esp = d->sp;
	proc_entry_t entry = d->entry;
	free(d);

	process_t *proc = current_thread->proc;
	ASSERT(proc != 0);

	switch_pagedir(proc->pd);

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

bool process_new_thread(process_t *p, proc_entry_t entry, void* sp) {
	setup_data_t *d = (setup_data_t*)malloc(sizeof(setup_data_t));
	d->entry = entry;
	d->sp = sp;

	thread_t *th = new_thread(run_user_code, d);
	if (th == 0) {
		return false;
	}

	th->proc = p;
	th->user_ex_handler = proc_user_exception;

	{	mutex_lock(&p->lock);

		th->next_in_proc = p->threads;
		p->threads = th;

		mutex_unlock(&p->lock);	}
	
	start_thread(th);

	return true;
}

bool start_process(process_t *p, void* entry) {
	bool stack_ok = mmap(p, (void*)USERSTACK_ADDR, USERSTACK_SIZE, MM_READ | MM_WRITE);
	if (!stack_ok) return false;

	bool ok = process_new_thread(p, entry, (void*)USERSTACK_ADDR + USERSTACK_SIZE);
	if (!ok) {
		munmap(p, (void*)USERSTACK_ADDR);
		return false;
	}

	p->status = PS_RUNNING;

	return true;
}

void current_process_exit(int status, int exit_code) {
	void process_exit_v(void* args) {
		exit_data_t *d = (exit_data_t*)args;
		process_exit(d->proc, d->status, d->exit_code);
		free(d);
	}

	exit_data_t *d = (exit_data_t*)malloc(sizeof(exit_data_t));

	d->proc = current_process();;
	d->status = status;
	d->exit_code = exit_code;

	worker_push(process_exit_v, d);
	exit();
}

void process_exit(process_t *p, int status, int exit_code) {
	// --- Make sure we are not running in a thread we are about to kill
	ASSERT(current_process() != p);

	// ---- Check we are not killing init process
	if (p->parent == 0) {
		PANIC("Attempted to exit init process!");
	}

	// ---- Now we can do the actual cleanup

	mutex_lock(&p->lock);

	ASSERT(p->status == PS_RUNNING || p->status == PS_LOADING);
	p->status = status;
	p->exit_code = exit_code;

	// neutralize the process
	while (p->threads != 0) {
		thread_t *t = p->threads;
		p->threads = p->threads->next_in_proc;

		t->proc = 0;		// we don't want process_thread_deleted to be called
		kill_thread(t);
	}

	// terminate all the children as well and free associated process_t structures
	while (p->children != 0) {
		process_t *ch = p->children;
		p->children = ch->next_child;

		if (ch->status == PS_RUNNING || ch->status == PS_LOADING)
			process_exit(ch, PS_KILLED, 0);
		free(ch);
	}

	// release file descriptors
	void release_fd(void* a, void* fd) {
		unref_file((fs_handle_t*)fd);
	}
	hashtbl_iter(p->files, release_fd);
	delete_hashtbl(p->files);
	p->files = 0;

	// unmap memory
	while (p->regions != 0) {
		munmap(p, p->regions->addr);
	}
	ASSERT(btree_count(p->regions_idx) == 0);
	delete_btree(p->regions_idx);
	p->regions_idx = 0;

	// release filesystems
	void release_fs(void* a, void* fs) {
		unref_fs((fs_t*)fs);
	}
	hashtbl_iter(p->filesystems, release_fs);
	delete_hashtbl(p->filesystems);
	p->filesystems = 0;

	// delete page directory
	switch_pagedir(get_kernel_pagedir());
	delete_pagedir(p->pd);
	p->pd = 0;

	// notify parent
	process_t *par = p->parent;
	if (par->status == PS_RUNNING) {
		resume_on(par->children);
		resume_on(p);
	}

	mutex_unlock(&p->lock);
}

void process_thread_deleted(thread_t *t) {
	process_t *p = t->proc;

	mutex_lock(&p->lock);

	if (p->threads == t) {
		p->threads = t->next_in_proc;
	} else {
		for (thread_t *it = p->threads; it != 0; it = it->next_in_proc) {
			if (it->next_in_proc == t) {
				it->next_in_proc = t->next_in_proc;
				break;
			}
		}
	}

	mutex_unlock(&p->lock);

	if (p->threads == 0 && p->status == PS_RUNNING)
		process_exit(p, PS_FINISHED, 0);
}

// =========================== //
// PROCESS CHILDREN MANAGEMENT //
// =========================== //

process_t *process_find_child(process_t *p, int pid) {
	process_t *ret = 0;
	
	mutex_lock(&p->lock);

	for (process_t *it = p->children; it != 0; it = it->next_child) {
		if (it->pid == pid) {
			ret = it;
			break;
		}
	}

	mutex_unlock(&p->lock);

	return ret;
}

void process_get_status(process_t *p, proc_status_t *st) {
	st->pid = p->pid;
	st->status = p->status;
	st->exit_code = p->exit_code;
}

void process_wait(process_t *p, proc_status_t *st, bool wait) {
	// This is such a mess...

	process_t *par = p->parent;

	st->pid = 0;
	st->status = 0;

	mutex_lock(&par->lock);

	while (true) {
		// problem : p may not be in parent's children list anymore,
		// meaning the associated process_t* has been deleted
		bool found = false;
		for (process_t *it = par->children; it != 0; it = it->next_child) {
			if (it == p) {
				found = true;
				break;
			}
		}

		if (found) {
			if (p->status != PS_LOADING && p->status != PS_RUNNING) {
				process_get_status(p, st);

				if (par->children == p) {
					par->children = p->next_child;
				} else {
					for (process_t *it = par->children; it->next_child != 0; it = it->next_child) {
						if (it->next_child == p) {
							it->next_child = p->next_child;
							break;
						}
					}
				}

				free(p);
			} else if (wait) {
				bool wait_ok;

				{	int st = enter_critical(CL_NOSWITCH);
		
					mutex_unlock(&par->lock);
					wait_ok = wait_on(p);

					exit_critical(st);	}
				
				if (!wait_ok) return;

				mutex_lock(&par->lock);
				continue;	// loop around
			}
		}
		break;
	}

	mutex_unlock(&par->lock);
}

void process_wait_any_child(process_t *par, proc_status_t *st, bool wait) {
	st->pid = 0;
	st->status = 0;

	mutex_lock(&par->lock);

	while (true) {
		// problem : p may not be in parent's children list anymore,
		// meaning the associated process_t* has been deleted
		process_t *p = 0;

		for (process_t *it = par->children; it != 0; it = it->next_child) {
			if (it->status != PS_LOADING && it->status != PS_RUNNING) {
				p = it;
				break;
			}
		}

		if (p) {
			process_get_status(p, st);

			if (par->children == p) {
				par->children = p->next_child;
			} else {
				for (process_t *it = par->children; it->next_child != 0; it = it->next_child) {
					if (it->next_child == p) {
						it->next_child = p->next_child;
						break;
					}
				}
			}

			free(p);
		} else {
			if (wait) {
				bool wait_ok;
				
				{	int st = enter_critical(CL_NOSWITCH);

					mutex_unlock(&par->lock);
					wait_ok = wait_on(par);

					exit_critical(st);	}
				
				if (!wait_ok) return;

				mutex_lock(&par->lock);
				continue;	// loop around
			}
		}
		break;
	}

	mutex_unlock(&par->lock);
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
	return (fs_t*)hashtbl_find(p->filesystems, name);
}

void proc_rm_fs(process_t *p, const char* name) {
	fs_t *fs = proc_find_fs(p, name);
	if (fs) {
		unref_fs(fs);
		hashtbl_remove(p->filesystems, name);
	}
}

int proc_add_fd(process_t *p, fs_handle_t *f) {
	int fd = p->next_fd++;

	bool add_ok = hashtbl_add(p->files, (void*)fd, f);
	if (!add_ok) return 0;

	return fd;
}

bool proc_add_fd_as(process_t *p, fs_handle_t *f, int fd) {
	if (hashtbl_find(p->files, (void*)fd) != 0) return false;

	if (fd >= p->next_fd) p->next_fd = fd + 1;

	bool add_ok = hashtbl_add(p->files, (void*)fd, f);
	return add_ok;
}

fs_handle_t *proc_read_fd(process_t *p, int fd) {
	return (fs_handle_t*)hashtbl_find(p->files, (void*)fd);
}

void proc_close_fd(process_t *p, int fd) {
	fs_handle_t *x = proc_read_fd(p, fd);
	if (x != 0) {
		unref_file(x);
		hashtbl_remove(p->files, (void*)fd);
	}
}

// ============================= //
// USER MEMORY REGION MANAGEMENT //
// ============================= //

static user_region_t *find_user_region(process_t *proc, const void* addr) {
	user_region_t *r = (user_region_t*)btree_lower(proc->regions_idx, addr);
	if (r == 0) return 0;

	ASSERT(addr >= r->addr);

	if (addr >= r->addr + r->size) return 0;
	return r;
}

bool mmap(process_t *proc, void* addr, size_t size, int mode) {
	if (find_user_region(proc, addr) != 0) return false;

	if ((uint32_t)addr & (~PAGE_MASK)) return false;

	if (addr >= (void*)K_HIGHHALF_ADDR || addr + size > (void*)K_HIGHHALF_ADDR
		|| addr + size <= addr || size == 0) return false;

	user_region_t *r = (user_region_t*)malloc(sizeof(user_region_t));
	if (r == 0) return false;

	r->proc = proc;
	r->addr = addr;
	r->size = size;

	r->own_pager = true;
	r->pager = new_swap_pager(size, false);
	if (r->pager == 0) goto error;
	r->offset = 0;

	bool add_ok = btree_add(proc->regions_idx, r->addr, r);
	if (!add_ok) goto error;

	r->mode = mode;
	r->file = 0;

	r->next_in_proc = proc->regions;
	proc->regions = r;

	pager_add_map(r->pager, r);

	return true;

error:
	if (r && r->pager) delete_pager(r->pager);
	if (r) free(r);
	return false;
}

bool mmap_file(process_t *proc, fs_handle_t *h, size_t offset, void* addr, size_t size, int mode) {
	if (find_user_region(proc, addr) != 0) return false;

	if ((uint32_t)addr & (~PAGE_MASK)) return false;
	if ((uint32_t)offset & (~PAGE_MASK)) return false;

	int fmode = file_get_mode(h);
	if (!(fmode & FM_MMAP) || !(fmode & FM_READ)) return false;
	if ((mode & MM_WRITE) && !(fmode & FM_WRITE)) return false;

	ASSERT(h->node->pager != 0);

	if (addr >= (void*)K_HIGHHALF_ADDR || addr + size > (void*)K_HIGHHALF_ADDR
		|| addr + size <= addr || size == 0) return false;

	user_region_t *r = (user_region_t*)malloc(sizeof(user_region_t));
	if (r == 0) return false;

	r->addr = addr;
	r->size = size;

	bool add_ok = btree_add(proc->regions_idx, r->addr, r);
	if (!add_ok) goto error;

	ref_file(h);
	r->file = h;

	r->mode = mode;
	r->offset = offset;
	r->pager = h->node->pager;
	r->own_pager = false;

	pager_add_map(r->pager, r);

	r->next_in_proc = proc->regions;
	proc->regions = r;

	return true;

error:
	if (r) free(r);
	return false;
}

bool mchmap(process_t *proc, void* addr, int mode) {
	user_region_t *r = find_user_region(proc, addr);

	if (r == 0) return false;
	
	if (r->file != 0) {
		if ((mode & MM_WRITE) && !(file_get_mode(r->file) & FM_WRITE)) return false;
	}
	r->mode = mode;

	// change mode on already mapped pages
	pagedir_t *save_pd = get_current_pagedir();
	switch_pagedir(proc->pd);
	for (void* it = r->addr; it < r->addr + r->size; it += PAGE_SIZE) {
		uint32_t ent = pd_get_entry(it);
		uint32_t frame = pd_get_frame(it);

		if (ent & PTE_PRESENT) {
			bool can_w = (ent & PTE_RW) != 0;
			bool should_w = (mode & MM_WRITE) != 0;
			if (can_w != should_w) {
				pd_map_page(it, frame, should_w);
			}
		}
	}
	switch_pagedir(save_pd);

	return true;
}

bool munmap(process_t *proc, void* addr) {
	user_region_t *r = find_user_region(proc, addr);
	if (r == 0) return false;

	if (proc->regions == r) {
		proc->regions = r->next_in_proc;
	} else {
		for (user_region_t *it = proc->regions; it != 0; it = it->next_in_proc) {
			if (it->next_in_proc == r) {
				it->next_in_proc = r->next_in_proc;
				break;
			}
		}
	}

	btree_remove_v(proc->regions_idx, r->addr, r);

	// Unmap that stuff
	pagedir_t *save_pd = get_current_pagedir();
	switch_pagedir(proc->pd);
	for (void* it = r->addr; it < r->addr + r->size; it += PAGE_SIZE) {
		uint32_t ent = pd_get_entry(it);

		if (ent & PTE_PRESENT) {
			pager_access(r->pager,
				it - r->addr + r->offset, PAGE_SIZE,
				(ent & PTE_ACCESSED), (ent & PTE_DIRTY));
			pd_unmap_page(it);
		}
	}
	switch_pagedir(save_pd);

	pager_rm_map(r->pager, r);

	if (r->file != 0) unref_file(r->file);
	if (r->own_pager) delete_pager(r->pager);

	free(r);

	return true;
}

void dbg_dump_proc_memmap(process_t *proc) {
	//WARNING not thread safe

	dbg_printf("/ Region map for process %d\n", proc->pid);

	for (user_region_t *it = proc->regions; it != 0; it = it->next_in_proc) {
		dbg_printf("| 0x%p - 0x%p : (0x%p) ", it->addr, it->addr + it->size, it->pager);
		if (it->file != 0) {
			dbg_printf("mmap of 0x%p", it->file);
		} else {
			dbg_printf("private");
		}
		dbg_printf(", %d pages\n", hashtbl_count(it->pager->pages));
	}
	
	dbg_printf("\\ ----\n");
}

// =============================== //
// USER MEMORY PAGE FAULT HANDLERS //
// =============================== //

void proc_user_exception(registers_t *regs) {
	dbg_printf("Usermode exception in user process : exiting.\n");
	dbg_dump_registers(regs);
	current_process_exit(PS_FAILURE, FAIL_EXCEPTION);
}
void proc_usermem_pf(void* p, registers_t *regs, void* addr) {
	process_t *proc = (process_t*)p;

	user_region_t *r = find_user_region(proc, addr);
	if (r == 0) {
		dbg_printf("Segmentation fault in process %d (0x%p : not mapped) : exiting.\n", proc->pid, addr);
		dbg_dump_registers(regs);
		current_process_exit(PS_FAILURE, (addr < (void*)PAGE_SIZE ? FAIL_ZEROPTR : FAIL_SEGFAULT));
	}

	bool wr = ((regs->err_code & PF_WRITE_BIT) != 0);
	if (wr && !(r->mode & MM_WRITE)) {
		dbg_printf("Segmentation fault in process %d (0x%p : not allowed to write) : exiting.\n", proc->pid, addr);
		current_process_exit(PS_FAILURE, (addr < (void*)PAGE_SIZE ? FAIL_ZEROPTR : FAIL_SEGFAULT));
	}

	bool pr = (regs->err_code & PF_PRESENT_BIT) != 0;
	if (pr) {
		uint32_t frame = pd_get_entry(addr);
		dbg_printf("UPF 0x%p %s present %d 0x%p\n",
			addr, (wr ? "write" : "read"),
			frame >> PTE_FRAME_SHIFT, frame);
		PANIC("Unexpected fault on present page.");
	}

	addr = (void*)((uint32_t)addr & PAGE_MASK);

	uint32_t frame;
	do {
		frame = pager_get_frame(r->pager, addr - r->addr + r->offset);
		if (frame == 0) {
			free_some_memory();
		}
	} while (frame == 0);

	while(!pd_map_page(addr, frame, (r->mode & MM_WRITE) != 0)) {
		free_some_memory();
	}
}

void probe_for_read(const void* addr, size_t len) {
	process_t *proc = current_process();
	user_region_t *r = find_user_region(proc, addr);
	if (r == 0 || addr + len > r->addr + r->size || !(r->mode & MM_READ)) {
		dbg_printf("Access violation on read at 0x%p len 0x%p in process %d : exiting.\n",
			addr, len, proc->pid);
		current_process_exit(PS_FAILURE, FAIL_SC_SEGFAULT);
	}
}

void probe_for_write(const void* addr, size_t len) {
	process_t *proc = current_process();
	user_region_t *r = find_user_region(proc, addr);
	if (r == 0 || addr + len > r->addr + r->size || !(r->mode & MM_WRITE)) {
		dbg_printf("Access violation on write at 0x%p len 0x%p in process %d : exiting.\n",
			addr, len, proc->pid);
		current_process_exit(PS_FAILURE, FAIL_SC_SEGFAULT);
	}
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
