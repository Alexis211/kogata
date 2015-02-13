#include <mutex.h>
#include <hashtbl.h>

#include <process.h>

typedef struct user_region {
	void* addr;
	size_t size;

	int mode;

	fs_handle_t *file;		// null if not mmaped-file
	size_t file_offset;

	struct user_region *next;
} user_region_t;

typedef struct process {
	pagedir_t *pd;
	user_region_t *regions;

	hashtbl_t *filesystems;

	thread_t *thread;

	int pid;
	struct process *parent;
} process_t;

static int next_pid = 1;

static void proc_kmem_violation(registers_t *regs);
static void proc_user_pf(void* proc, registers_t *regs, void* addr);

process_t *current_process() {
	if (current_thread) return current_thread->proc;
	return 0;
}

// ============================== //
// CREATING AND RUNNING PROCESSES //
// ============================== //

process_t *new_process(process_t *parent) {
	process_t *proc = (proces_t*)malloc(sizeof(process_t));
	if (proc == 0) return 0;

	proc->filesystems = create_hashtbl(str_key_eq_fun, str_hash_fun, free, 0);
	if (proc->filesystems == 0) {
		free(proc);
		return 0;
	}

	proc->pd = pagedir_create(proc_user_pf, proc);
	if (proc->pd == 0) {
		delete_hashtbl(proc->filesystems, 0);
		free(proc);
		return 0;
	}

	proc->regions = 0;
	proc->thread = 0;
	proc->pid = (next_pid++);
	proc->parent = parent;

	return proc;
}

static void run_user_code(void* data) {
	// TODO
	exit();
}
bool start_process(process_t *p, void* entry) {
	// TODO
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

bool mmap(process_t *proc, void* addr, size_t size, int mode) {
	//TODO
	return false;
}

bool mmap_file(process_t *proc, fs_handle_t *h, size_t offset, void* addr, size_t size, int mode) {
	//TODO
	return false;
}

bool mchmap(process_t *proc, void* addr, int mode) {
	//TODO
	return false;
}

bool munmap(process_t *proc, void* addr) {
	//TODO
	return false;
}

// =============================== //
// USER MEMORY PAGE FAULT HANDLERS //
// =============================== //

static void proc_kmem_violation(registers_t *regs) {
	//TODO
	dbg_printf("Kernel memory violation in user process : exiting.\n");
	exit();
}
static void proc_user_pf(void* proc, registers_t *regs, void* addr) {
	//TODO
	PANIC("TODO");
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
