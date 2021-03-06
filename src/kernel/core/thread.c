#include <thread.h>
#include <malloc.h>
#include <dbglog.h>
#include <idt.h>
#include <gdt.h>

#include <hashtbl.h>

#include <frame.h>
#include <paging.h>
#include <worker.h>
#include <process.h>
#include <freemem.h>
#include <prng.h>

void save_context_and_enter_scheduler(saved_context_t *ctx);
void resume_context(saved_context_t *ctx);

thread_t *current_thread = 0;

static thread_t *waiters = 0;

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

	/*dbg_printf(" >%d< ", current_thread->critical_level);*/

	return prev_level;
}

void exit_critical(int prev_level) {
	asm volatile("cli");

	if (current_thread == 0) return;

	if (prev_level < current_thread->critical_level) current_thread->critical_level = prev_level;
	if (current_thread->critical_level < CL_NOINT) asm volatile("sti");

	/*dbg_printf(" <%d> ", current_thread->critical_level);*/
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

	thread_t *prev_thread = current_thread;

	if (current_thread != 0 && current_thread->state == T_STATE_RUNNING) {
		current_thread->last_ran = get_kernel_time();
		if (current_thread->proc) current_thread->proc->last_ran = current_thread->last_ran;
		enqueue_thread(current_thread, true);
	}
	current_thread = 0;

	thread_t *next_thread = dequeue_thread();

	if (next_thread != prev_thread && SPAM_CONTEXT_SWITCH) dbg_printf("[0x%p]\n", next_thread);

	if (next_thread != 0) {
		prng_add_entropy((uint8_t*)&next_thread, sizeof(next_thread));

		set_kernel_stack(next_thread->stack_region->addr + next_thread->stack_region->size);

		current_thread = next_thread;
		resume_context(&current_thread->ctx);
	} else {
		// Wait for an IRQ
		asm volatile("sti; hlt; cli");
		// At this point an IRQ has happenned
		// and has been processed. Loop around. 
		run_scheduler();
	}
}

void run_thread(void (*entry)(void*), void* data) {
	ASSERT(current_thread->state == T_STATE_RUNNING);

	if (SPAM_BEGIN_EXIT) dbg_printf("Begin thread 0x%p (in process %d)\n",
		current_thread, (current_thread->proc ? current_thread->proc->pid : 0));

	switch_pagedir(get_kernel_pagedir());

	exit_critical(CL_USER);

	entry(data);

	exit();
}
thread_t *new_thread(entry_t entry, void* data) {
	thread_t *t = (thread_t*)malloc(sizeof(thread_t));
	if (t == 0) return 0;

	void* stack = region_alloc(KPROC_STACK_SIZE + PAGE_SIZE, "Stack");
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
			if (SPAM_OOM_REASON) dbg_printf("OOM when allocating thread stack\n");
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
	t->state = T_STATE_LOADING;
	t->last_ran = 0;

	t->must_exit = false;

	t->current_pd_d = get_kernel_pagedir();
	t->critical_level = CL_EXCL;

	// used by user processes
	t->proc = 0;
	t->next_in_proc = 0;
	t->user_ex_handler = 0;

	t->waiting_on = 0;
	t->n_waiting_on = 0;
	t->next_waiter = 0;

	return t;
}

void delete_thread(thread_t *t) {
	ASSERT(t->state == T_STATE_FINISHED);

	if (SPAM_BEGIN_EXIT) dbg_printf("Deleting thread 0x%p\n", t);

	region_free_unmap_free(t->stack_region->addr);
	free(t);
}

// ========== //
// SETUP CODE //
// ========== //

void irq0_handler(registers_t *regs) {
	notify_time_pass(1000000 / TASK_SWITCH_FREQUENCY);
}
void threading_irq0_handler() {
	if (current_thread != 0 && current_thread->critical_level == CL_USER) {
		save_context_and_enter_scheduler(&current_thread->ctx);
	}
}
void threading_setup(entry_t cont, void* arg) {
	set_pit_frequency(TASK_SWITCH_FREQUENCY);
	idt_set_irq_handler(IRQ0, &irq0_handler);

	thread_t *t = new_thread(cont, arg);
	ASSERT(t != 0);

	start_thread(t);
	run_scheduler();	// never returns
	ASSERT(false);
}

// ======================= //
// TASK STATE MANIPULATION //
// ======================= //

void start_thread(thread_t *t) {
	ASSERT(t->state == T_STATE_LOADING);

	t->state = T_STATE_RUNNING;

	{	int st = enter_critical(CL_NOINT);

		enqueue_thread(t, false);

		exit_critical(st);	}
}

void yield() {
	ASSERT(current_thread != 0);
	ASSERT(current_thread->critical_level != CL_EXCL);

	save_context_and_enter_scheduler(&current_thread->ctx);
}

bool wait_on(void* x) {
	return wait_on_many(&x, 1);
}

bool wait_on_many(void** x, size_t n) {
	ASSERT(current_thread != 0);
	ASSERT(current_thread->critical_level != CL_EXCL);
	ASSERT(n > 0);

	int st = enter_critical(CL_NOINT);

	//  ---- Set ourselves as the waiting thread for all the requested objets

	if (SPAM_WAIT_RESUME_ON) {
		dbg_printf("Wait on many:");
		for (size_t i = 0; i < n; i++) {
			dbg_printf(" 0x%p", x[i]);
		}
		dbg_printf("\n");
	}

	current_thread->waiting_on = x;
	current_thread->n_waiting_on = n;

	current_thread->next_waiter = waiters;
	waiters = current_thread;

	//  ---- Go to sleep

	current_thread->state = T_STATE_PAUSED;
	save_context_and_enter_scheduler(&current_thread->ctx);

	//  ---- Remove ourselves from the list

	current_thread->waiting_on = 0;
	current_thread->n_waiting_on = 0;

	if (waiters == current_thread) {
		waiters = current_thread->next_waiter;
	} else {
		ASSERT(waiters != 0);
		bool deleted = false;
		for (thread_t *w = waiters; w->next_waiter != 0; w = w->next_waiter) {
			if (w->next_waiter == current_thread) {
				w->next_waiter = current_thread->next_waiter;
				deleted = true;
				break;
			}
		}
		ASSERT(deleted);
	}

	exit_critical(st);

	//  ---- Check that we weren't waked up because of a kill request
	if (current_thread->must_exit) return false;

	return true;
}

void _usleep_resume_on_v(void* x) {
	resume_on(x);
}
void usleep(int usecs) {
	if (current_thread == 0) return;

	bool ok = worker_push_in(usecs, _usleep_resume_on_v, current_thread);

	if (ok) wait_on(current_thread);
}

void exit_cleanup_task(void* v) {
	thread_t *t = (thread_t*)v;

	if (t->proc == 0) {
		// stand alone thread, can be deleted safely
		delete_thread(t);
	} else {
		// call specific routine from process code
		process_thread_exited(t);
	}
}

void exit() {
	int st = enter_critical(CL_NOSWITCH);
	// the critical section here does not guarantee that worker_push will return immediately
	// (it may switch before adding the delete_thread task), but once the task is added
	// no other switch may happen, therefore this thread will not get re-enqueued

	if (SPAM_BEGIN_EXIT) dbg_printf("Thread 0x%p exiting.\n", current_thread);

	worker_push(exit_cleanup_task, current_thread);

	current_thread->state = T_STATE_FINISHED;
	exit_critical(st);

	yield();	// expected never to return!
	ASSERT(false);
}

bool resume_on(void* x) {
	bool ret = false;

	int st = enter_critical(CL_NOINT);

	if (SPAM_WAIT_RESUME_ON) dbg_printf("Resume on 0x%p:", x);

	for (thread_t *t = waiters; t != 0; t = t->next_waiter) {
		for (int i = 0; i < t->n_waiting_on; i++) {
			if (t->waiting_on[i] == x) {
				if (SPAM_WAIT_RESUME_ON) dbg_printf(" 0x%p", t);

				if (t->state == T_STATE_PAUSED) {
					t->state = T_STATE_RUNNING;

					enqueue_thread(t, false);

					ret = true;
				}
				break;
			}
		}
	}
	if (SPAM_WAIT_RESUME_ON) dbg_printf("\n");

	exit_critical(st);

	return ret;
}

void kill_thread(thread_t *thread) {
	ASSERT(thread != current_thread);

	int st = enter_critical(CL_NOSWITCH);

	thread->must_exit = true;

	int i = 0;
	while (thread->state != T_STATE_FINISHED) {
		if (thread->state == T_STATE_PAUSED) {
			thread->state = T_STATE_RUNNING;
			enqueue_thread(thread, false);
		}
		yield();
		if (i++ == 100) dbg_printf("Thread 0x%p must be killed but will not exit.\n", thread);
	}

	exit_critical(st);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
