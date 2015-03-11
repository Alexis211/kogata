#pragma once

#include <sys.h>
#include <paging.h>
#include <region.h>
#include <idt.h>

#define T_STATE_LOADING		0
#define T_STATE_RUNNING		1
#define T_STATE_PAUSED	 	2
#define T_STATE_FINISHED	3

#define KPROC_STACK_SIZE 0x2000	// 8Kb

#define TASK_SWITCH_FREQUENCY	100		// in herz

typedef struct saved_context {
	uint32_t *esp;
	void (*eip)();
} saved_context_t;

typedef struct process process_t;
typedef struct thread {
	saved_context_t ctx;
	pagedir_t *current_pd_d;

	uint32_t state;
	uint64_t last_ran;
	int critical_level;

	region_info_t *stack_region;

	process_t *proc;
	isr_handler_t user_ex_handler;	// exception in user code

	struct thread *next_in_queue;
	struct thread *next_in_proc;

	void** waiting_on;
	int n_waiting_on;
	struct thread *next_waiter;

	bool must_exit;
} thread_t;

typedef void (*entry_t)(void*);

void threading_setup(entry_t cont, void* data);		// never returns
thread_t *new_thread(entry_t entry, void* data);	// thread is PAUSED, and must be started with start_thread
void start_thread(thread_t *t);
void delete_thread(thread_t *t);

void threading_irq0_handler();

extern thread_t *current_thread;

void yield();
void exit();
void usleep(int usecs);

// Important notice about waiting, resuming and killing :
//  Threads may now communicate via wait_on(void* ressource) and resume_on (void* ressource).
//  Previous pause() is replaced by wait_on(current_thread) and resume(thread) by resume_on(thread).
//  wait_on(x) may return false, indicating that the reason for returning is NOT that resume_on(x) was
//  called but something else happenned. Typically false indicates that the curent thread is being
//  killed and must terminate its kernel-land processing as soon as possible.

bool wait_on(void* x);	// true : resumed normally, false : resumed because thread was killed, or someone else already waiting
bool wait_on_many(void** x, size_t count);	// true only if we could wait on ALL objects

bool resume_on(void* x);
void kill_thread(thread_t *thread);		// cannot be called for current thread

// Kernel critical sections
#define CL_EXCL			3	// No interruptions accepted, context switching not allowed
#define CL_NOINT 		2	// No interruptions accepted, timer context switching disabled (manual switch allowed)
#define CL_NOSWITCH 	1	// Interruptions accepted, timer context switching disabled (manual switch allowed)
#define CL_USER  		0	// Anything can happen

int enter_critical(int level);
void exit_critical(int prev_level);

/* vim: set ts=4 sw=4 tw=0 noet :*/
