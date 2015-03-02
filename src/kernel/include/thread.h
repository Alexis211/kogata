#pragma once

#include <sys.h>
#include <paging.h>
#include <region.h>
#include <idt.h>

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
	isr_handler_t user_ex_handler;	// page fault in kernel memory accessed by user code (violation)

	struct thread *next_in_queue;
} thread_t;

typedef void (*entry_t)(void*);

void threading_setup(entry_t cont, void* data);		// never returns
thread_t *new_thread(entry_t entry, void* data);	// thread is PAUSED, and must be resume_thread'ed

extern thread_t *current_thread;

void yield();
void pause();
void exit();
void usleep(int usecs);

bool resume_thread(thread_t *thread);
void kill_thread(thread_t *thread);

// Kernel critical sections
#define CL_EXCL			3	// No interruptions accepted, context switching not allowed
#define CL_NOINT 		2	// No interruptions accepted, timer context switching disabled (manual switch allowed)
#define CL_NOSWITCH 	1	// Interruptions accepted, timer context switching disabled (manual switch allowed)
#define CL_USER  		0	// Anything can happen

int enter_critical(int level);
void exit_critical(int prev_level);

/* vim: set ts=4 sw=4 tw=0 noet :*/
