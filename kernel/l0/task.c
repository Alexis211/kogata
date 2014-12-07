#include <task.h>
#include <kmalloc.h>
#include <dbglog.h>
#include <idt.h>

#include <frame.h>
#include <paging.h>

void save_context_and_enter_scheduler(saved_context_t *ctx);
void irq0_save_context_and_enter_scheduler(saved_context_t *ctx);
void resume_context(saved_context_t *ctx);

task_t *current_task = 0;

// ====================== //
// THE PROGRAMMABLE TIMER //
// ====================== //

void set_pit_frequency(uint32_t freq) {
   uint32_t divisor = 1193180 / freq;
   ASSERT(divisor < 65536);	// must fit on 16 bits

   uint8_t l = (divisor & 0xFF);
   uint8_t h = ((divisor >> 8) & 0xFF);

   outb(0x43, 0x36);
   outb(0x40, l);
   outb(0x40, h);
}

// ============================= //
// HELPER : IF FLAG MANIPULATION //
// ============================= //

static inline bool disable_interrupts() {
	uint32_t eflags;
	asm volatile("pushf; pop %0" : "=r"(eflags));
	asm volatile("cli");
	return (eflags & EFLAGS_IF) != 0;
}

static inline void resume_interrupts(bool st) {
	if (st) asm volatile("sti");
}

// ================== //
// THE TASK SCHEDULER //
// ================== //

static task_t *queue_first_task = 0, *queue_last_task = 0;

void enqueue_task(task_t *t, bool just_ran) {
	ASSERT(t->state == T_STATE_RUNNING);
	if (queue_first_task == 0) {
		queue_first_task = queue_last_task = t;
		t->next_in_queue = 0;
	} else if (just_ran) {
		t->next_in_queue = 0;
		queue_last_task->next_in_queue = t;
		queue_last_task = t;
	} else {
		t->next_in_queue = queue_first_task;
		queue_first_task = t;
	}
}

task_t* dequeue_task() {
	task_t *t = queue_first_task;
	if (t == 0) return 0;

	queue_first_task = t->next_in_queue;
	if (queue_first_task == 0) queue_last_task = 0;

	return t;
}

// ================ //
// THE TASKING CODE //
// ================ //

void run_scheduler() {
	// At this point, interrupts are disabled
	// This function is expected NEVER TO RETURN

	if (current_task != 0 && current_task->state == T_STATE_RUNNING) {
		enqueue_task(current_task, true);
	}

	current_task = dequeue_task();
	if (current_task != 0) {
		resume_context(&current_task->ctx);
	} else {
		// Wait for an IRQ
		asm volatile("sti; hlt");
		// At this point an IRQ has happenned
		// and has been processed. Loop around. 
		run_scheduler();
		ASSERT(false);
	}
}

static void run_task(void (*entry)(void*)) {
	ASSERT(current_task->state == T_STATE_RUNNING);
	ASSERT(current_task->has_result);

	switch_pagedir(get_kernel_pagedir());

	current_task->has_result = false;
	
	asm volatile("sti");
	entry(current_task->result);

	current_task->state = T_STATE_FINISHED;
	// TODO : add job for deleting the task, or whatever
	yield();	// expected never to return!
	ASSERT(false);
}
task_t *new_task(entry_t entry) {
	task_t *t = (task_t*)kmalloc(sizeof(task_t));
	if (t == 0) return 0;

	void* stack = region_alloc(KPROC_STACK_SIZE, REGION_T_KPROC_STACK, 0);
	if (stack == 0) {
		kfree(t);
		return 0;
	}

	for (void* i = stack + PAGE_SIZE; i < stack + KPROC_STACK_SIZE; i += PAGE_SIZE) {
		uint32_t f = frame_alloc(1);
		if (f == 0) {
			region_free_unmap_free(stack);
			kfree(t);
			return 0;
		}
		pd_map_page(i, f, true);
	}

	t->stack_region = find_region(stack);

	t->ctx.esp = (uint32_t*)(t->stack_region->addr + t->stack_region->size);
	*(--t->ctx.esp) = (uint32_t)entry;	// push first argument : entry point
	*(--t->ctx.esp) = 0;		// push invalid return address (the run_task function never returns)

	t->ctx.eip = (void(*)())run_task;
	t->state = T_STATE_WAITING;
	t->result = 0;
	t->has_result = false;

	t->current_pd_d = get_kernel_pagedir();

	t->more_data = 0;	// free for use by L1 functions

	return t;
}

// ========== //
// SETUP CODE //
// ========== //

static void irq0_handler(registers_t *regs) {
	if (current_task != 0)
		irq0_save_context_and_enter_scheduler(&current_task->ctx);
}
void tasking_setup(entry_t cont, void* arg) {
	set_pit_frequency(TASK_SWITCH_FREQUENCY);
	idt_set_irq_handler(IRQ0, irq0_handler);

	task_t *t = new_task(cont);
	ASSERT(t != 0);

	resume_with_result(t, arg, false);

	run_scheduler();	// never returns
	ASSERT(false);
}

// ======================= //
// TASK STATE MANIPULATION //
// ======================= //

void yield() {
	if (current_task == 0) {
		// might happen before tasking is initialized
		// (but should not...)
		dbg_printf("Warning: probable deadlock.");
	} else {
		save_context_and_enter_scheduler(&current_task->ctx);
	}
}

void* wait_for_result() {
	bool st = disable_interrupts();

	if (!current_task->has_result) {
		current_task->state = T_STATE_WAITING;
		save_context_and_enter_scheduler(&current_task->ctx);
	}
	ASSERT(current_task->has_result);
	current_task->has_result = false;

	void *result = current_task->result;

	resume_interrupts(st);
	return result;
}

void resume_with_result(task_t *task, void* data, bool run_at_once) {
	bool st = disable_interrupts();

	task->has_result = true;
	task->result = data;

	if (task->state == T_STATE_WAITING) {
		task->state = T_STATE_RUNNING;
		enqueue_task(task, false);
	}
	if (run_at_once) yield();

	resume_interrupts(st);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
