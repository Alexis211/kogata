#include <thread.h>
#include <kmalloc.h>
#include <dbglog.h>
#include <idt.h>

#include <frame.h>
#include <paging.h>

void save_context_and_enter_scheduler(saved_context_t *ctx);
void irq0_save_context_and_enter_scheduler(saved_context_t *ctx);
void resume_context(saved_context_t *ctx);

thread_t *current_thread = 0;

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

static thread_t *queue_first_thread = 0, *queue_last_thread = 0;

void enqueue_thread(thread_t *t, bool just_ran) {
	ASSERT(t->state == T_STATE_RUNNING);
	if (queue_first_thread == 0) {
		queue_first_thread = queue_last_thread = t;
		t->next_in_queue = 0;
	} else if (just_ran) {
		t->next_in_queue = 0;
		queue_last_thread->next_in_queue = t;
		queue_last_thread = t;
	} else {
		t->next_in_queue = queue_first_thread;
		queue_first_thread = t;
	}
}

thread_t* dequeue_thread() {
	thread_t *t = queue_first_thread;
	if (t == 0) return 0;

	queue_first_thread = t->next_in_queue;
	if (queue_first_thread == 0) queue_last_thread = 0;

	return t;
}

// ================ //
// THE TASKING CODE //
// ================ //

void run_scheduler() {
	// At this point, interrupts are disabled
	// This function is expected NEVER TO RETURN

	if (current_thread != 0 && current_thread->state == T_STATE_RUNNING) {
		enqueue_thread(current_thread, true);
	}

	current_thread = dequeue_thread();
	if (current_thread != 0) {
		resume_context(&current_thread->ctx);
	} else {
		// Wait for an IRQ
		asm volatile("sti; hlt");
		// At this point an IRQ has happenned
		// and has been processed. Loop around. 
		run_scheduler();
		ASSERT(false);
	}
}

static void run_thread(void (*entry)(void*), void* data) {
	ASSERT(current_thread->state == T_STATE_RUNNING);

	switch_pagedir(get_kernel_pagedir());

	asm volatile("sti");
	entry(data);

	current_thread->state = T_STATE_FINISHED;
	// TODO : add job for deleting the thread, or whatever
	yield();	// expected never to return!
	ASSERT(false);
}
thread_t *new_thread(entry_t entry, void* data) {
	thread_t *t = (thread_t*)kmalloc(sizeof(thread_t));
	if (t == 0) return 0;

	void* stack = region_alloc(KPROC_STACK_SIZE, "Stack", 0);
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
	*(--t->ctx.esp) = (uint32_t)data;	// push second argument : data
	*(--t->ctx.esp) = (uint32_t)entry;	// push first argument : entry point
	*(--t->ctx.esp) = 0;		// push invalid return address (the run_thread function never returns)

	t->ctx.eip = (void(*)())run_thread;
	t->state = T_STATE_PAUSED;

	t->current_pd_d = get_kernel_pagedir();

	t->proc = 0;	// used by L1 functions

	return t;
}

// ========== //
// SETUP CODE //
// ========== //

static void irq0_handler(registers_t *regs) {
	if (current_thread != 0)
		irq0_save_context_and_enter_scheduler(&current_thread->ctx);
}
void threading_setup(entry_t cont, void* arg) {
	set_pit_frequency(TASK_SWITCH_FREQUENCY);
	idt_set_irq_handler(IRQ0, irq0_handler);

	thread_t *t = new_thread(cont, arg);
	ASSERT(t != 0);

	resume_thread(t, false);

	run_scheduler();	// never returns
	ASSERT(false);
}

// ======================= //
// TASK STATE MANIPULATION //
// ======================= //

void yield() {
	if (current_thread == 0) {
		// might happen before threading is initialized
		// (but should not...)
		dbg_printf("Warning: probable deadlock.\n");
	} else {
		save_context_and_enter_scheduler(&current_thread->ctx);
	}
}

void pause() {
	bool st = disable_interrupts();

	current_thread->state = T_STATE_PAUSED;
	save_context_and_enter_scheduler(&current_thread->ctx);

	resume_interrupts(st);
}

void resume_thread(thread_t *thread, bool run_at_once) {
	bool st = disable_interrupts();

	if (thread->state == T_STATE_PAUSED) {
		thread->state = T_STATE_RUNNING;
		enqueue_thread(thread, false);
	}
	if (run_at_once) yield();

	resume_interrupts(st);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
