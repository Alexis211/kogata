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

typedef struct task {
	saved_context_t ctx;
	pagedir_t *current_pd_d;

	uint32_t state;
	void* result;
	bool has_result;

	region_info_t *stack_region;

	void* more_data;

	struct task *next_in_queue;
} task_t;

typedef void (*entry_t)(void*);

void tasking_setup(entry_t cont, void* data);		// never returns
task_t *new_task(entry_t entry);	// task is PAUSED, and must be resume_task_with_result'ed

extern task_t *current_task;

void yield();
void* wait_for_result();

void resume_task_with_result(task_t *task, void* data, bool run_at_once);

/* vim: set ts=4 sw=4 tw=0 noet :*/
