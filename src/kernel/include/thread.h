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

	void* waiting_on;
	bool must_exit;
} thread_t;

typedef void (*entry_t)(void*);

void threading_setup(entry_t cont, void* data);		// never returns
thread_t *new_thread(entry_t entry, void* data);	// thread is PAUSED, and must be started with start_thread
void start_thread(thread_t *t);

extern thread_t *current_thread;

void yield();
void exit();
void usleep(int usecs);
bool wait_on(void* x);	// true : resumed normally, false : resumed because thread was killed, or someone else already waiting

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
