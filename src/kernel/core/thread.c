#include <thread.h>
#include <malloc.h>
#include <dbglog.h>
#include <idt.h>
#include <gdt.h>

#include <frame.h>
#include <paging.h>
#include <worker.h>
#include <process.h>
#include <freemem.h>

void save_context_and_enter_scheduler(saved_context_t *ctx);
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

// =========================== //
// CRITICAL SECTION MANAGEMENT //
// =========================== //

int enter_critical(int level) {
	asm volatile("cli");

	if (current_thread == 0) return CL_EXCL;

	int prev_level = current_thread->critical_level;
	if (level > prev_level) current_thread->critical_level = level;
	
	if (current_thread->critical_level < CL_NOINT) asm volatile("sti");

	return prev_level;
}

void exit_critical(int prev_level) {
	asm volatile("cli");

	if (current_thread == 0) return;

	if (prev_level < current_thread->critical_level) current_thread->critical_level = prev_level;
	if (current_thread->critical_level < CL_NOINT) asm volatile("sti");
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

void remove_thread_from_queue(thread_t *t) {
	if (queue_first_thread == t) {
		queue_first_thread = t->next_in_queue;
		if (queue_first_thread == 0) queue_last_thread = 0;
	} else {
		for (thread_t *it = queue_first_thread; it != 0; it = it->next_in_queue) {
			if (it->next_in_queue == t) {
				it->next_in_queue = t->next_in_queue;
				if (it->next_in_queue == 0) queue_last_thread = t;
				break;
			}
		}
	}
}

// ================ //
// THE TASKING CODE //
// ================ //

void run_scheduler() {
	// At this point, interrupts are disabled
	// This function is expected NEVER TO RETURN

	if (current_thread != 0 && current_thread->state == T_STATE_RUNNING) {
		current_thread->last_ran = worker_get_time();
		if (current_thread->proc) current_thread->proc->last_ran = current_thread->last_ran;
		enqueue_thread(current_thread, true);
	}
	current_thread = dequeue_thread();

	if (current_thread != 0) {
		set_kernel_stack(current_thread->stack_region->addr + current_thread->stack_region->size);
		resume_context(&current_thread->ctx);
	} else {
		// Wait for an IRQ
		asm volatile("sti; hlt");
		// At this point an IRQ has happenned
		// and has been processed. Loop around. 
		run_scheduler();
	}
}

static void run_thread(void (*entry)(void*), void* data) {
	ASSERT(current_thread->state == T_STATE_RUNNING);

	dbg_printf("Begin thread 0x%p (in process %d)\n",
		current_thread, (current_thread->proc ? current_thread->proc->pid : 0));

	switch_pagedir(get_kernel_pagedir());

	asm volatile("sti");
	entry(data);

	exit();
}
thread_t *new_thread(entry_t entry, void* data) {
	thread_t *t = (thread_t*)malloc(sizeof(thread_t));
	if (t == 0) return 0;

	void* stack = region_alloc(KPROC_STACK_SIZE + PAGE_SIZE, "Stack", pf_handler_stackoverflow);
	if (stack == 0) {
		free(t);
		return 0;
	}
	void* stack_low = stack + PAGE_SIZE;
	void* stack_high = stack_low + KPROC_STACK_SIZE;

	for (void* i = stack_low; i < stack_high; i += PAGE_SIZE) {
		uint32_t f;
		int tries = 0;
		while ((f = frame_alloc(1)) == 0 && (tries++) < 3) {
			free_some_memory();
		}
		if (f == 0) {
			PANIC("TODO (OOM could not create kernel stack for new thread)");
		}

		bool map_ok = pd_map_page(i, f, true);
		if (!map_ok) {
			PANIC("TODO (OOM(2) could not create kernel stack for new thread)");
		}
	}

	t->stack_region = find_region(stack);
	ASSERT(stack_high == t->stack_region->addr + t->stack_region->size);

	t->ctx.esp = (uint32_t*)stack_high;
	*(--t->ctx.esp) = (uint32_t)data;	// push second argument : data
	*(--t->ctx.esp) = (uint32_t)entry;	// push first argument : entry point
	*(--t->ctx.esp) = 0;		// push invalid return address (the run_thread function never returns)

	t->ctx.eip = (void(*)())run_thread;
	t->state = T_STATE_PAUSED;
	t->last_ran = 0;

	t->current_pd_d = get_kernel_pagedir();
	t->critical_level = CL_USER;

	// used by user processes
	t->proc = 0;
	t->next_in_proc = 0;
	t->user_ex_handler = 0;

	return t;
}

static void delete_thread(thread_t *t) {
	if (t->proc != 0)
		process_thread_deleted(t);

	region_free_unmap_free(t->stack_region->addr);
	free(t);
}

// ========== //
// SETUP CODE //
// ========== //

static void irq0_handler(registers_t *regs) {
	worker_notify_time(1000000 / TASK_SWITCH_FREQUENCY);
	if (current_thread != 0 && current_thread->critical_level == CL_USER) {
		save_context_and_enter_scheduler(&current_thread->ctx);
	}
}
void threading_setup(entry_t cont, void* arg) {
	set_pit_frequency(TASK_SWITCH_FREQUENCY);
	idt_set_irq_handler(IRQ0, irq0_handler);

	thread_t *t = new_thread(cont, arg);
	ASSERT(t != 0);

	resume_thread(t);
	exit_critical(CL_USER);

	run_scheduler();	// never returns
	ASSERT(false);
}

// ======================= //
// TASK STATE MANIPULATION //
// ======================= //

void yield() {
	ASSERT(current_thread != 0 && current_thread->critical_level != CL_EXCL);

	save_context_and_enter_scheduler(&current_thread->ctx);
}

void pause() {
	ASSERT(current_thread != 0 && current_thread->critical_level != CL_EXCL);

	current_thread->state = T_STATE_PAUSED;
	save_context_and_enter_scheduler(&current_thread->ctx);
}

void usleep(int usecs) {
	void sleeper_resume(void* t) {
		thread_t *thread = (thread_t*)t;
		resume_thread(thread);
	}
	if (current_thread == 0) return;
	bool ok = worker_push_in(usecs, sleeper_resume, current_thread);
	if (ok) pause();
}

void exit() {
	void delete_thread_v(void* v) {
		delete_thread((thread_t*)v);
	}

	int st = enter_critical(CL_NOSWITCH);
	// the critical section here does not guarantee that worker_push will return immediately
	// (it may switch before adding the delete_thread task), but once the task is added
	// no other switch may happen, therefore this thread will not get re-enqueued

	worker_push(delete_thread_v, current_thread);
	current_thread->state = T_STATE_FINISHED;

	exit_critical(st);

	yield();	// expected never to return!
	ASSERT(false);
}

bool resume_thread(thread_t *thread) {
	bool ret = false;

	int st = enter_critical(CL_NOINT);

	if (thread->state == T_STATE_PAUSED) {
		ret = true;
		thread->state = T_STATE_RUNNING;
		enqueue_thread(thread, false);
	}

	exit_critical(st);

	return ret;
}

void kill_thread(thread_t *thread) {
	ASSERT(thread != current_thread);

	int st = enter_critical(CL_NOSWITCH);

	thread->state = T_STATE_FINISHED;
	remove_thread_from_queue(thread);
	delete_thread(thread);

	exit_critical(st);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
