#pragma once

#include <sys.h>
#include <paging.h>
#include <region.h>

#define T_STATE_RUNNING		1
#define T_STATE_FINISHED	2
#define T_STATE_WAITING		3

#define KPROC_STACK_SIZE 0x8000	// 8Kb

#define TASK_SWITCH_FREQUENCY	100		// in herz

typedef struct saved_context {
	uint32_t *esp;
	void (*eip)();
} saved_context_t;

typedef struct thread {
	saved_context_t ctx;
	pagedir_t *current_pd_d;

	uint32_t state;
	void* result;
	bool has_result;

	region_info_t *stack_region;

	void* more_data;

	struct thread *next_in_queue;
} thread_t;

typedef void (*entry_t)(void*);

void threading_setup(entry_t cont, void* data);		// never returns
thread_t *new_thread(entry_t entry);	// thread is PAUSED, and must be resume_thread_with_result'ed

extern thread_t *current_thread;

void yield();
void* wait_for_result();

void resume_thread_with_result(thread_t *thread, void* data, bool run_at_once);

/* vim: set ts=4 sw=4 tw=0 noet :*/