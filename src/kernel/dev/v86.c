#include <string.h>
#include <paging.h>

#include <dev/v86.h>

#define EFLAGS_VM 0x20000
#define V86_VALID_FLAGS 0xDFF

//  ---- Ugly big static data

STATIC_MUTEX(v86_mutex);

v86_regs_t v86_regs;
bool v86_if;

pagedir_t *v86_pagedir = 0;
void* v86_alloc_addr;

bool v86_retval;
thread_t *v86_caller_thread = 0;

thread_t *v86_thread = 0;
uint32_t v86_int_no;

pagedir_t *v86_prev_pagedir = 0;

//  ---- Setup code

void v86_thread_main(void*);
void v86_ex_handler(registers_t *regs);
void v86_pf_handler(void*, registers_t *regs, void* addr);
void v86_asm_enter_v86(v86_regs_t*);

bool v86_begin_session() {
	mutex_lock(&v86_mutex);

	if (v86_pagedir == 0) {
		v86_pagedir = create_pagedir(v86_pf_handler, 0);
		if (v86_pagedir == 0) return false;

		v86_prev_pagedir = get_current_pagedir();
		switch_pagedir(v86_pagedir);

		for (void* addr = (void*)V86_ALLOC_ADDR; addr < (void*)V86_STACK_TOP; addr += PAGE_SIZE) {
			pd_map_page(addr, (uint32_t)addr / PAGE_SIZE, true);
		}
		for (void* addr = (void*)V86_BIOS_BEGIN; addr < (void*)V86_BIOS_END; addr += PAGE_SIZE) {
			pd_map_page(addr, (uint32_t)addr / PAGE_SIZE, true);
		}
		pd_map_page(0, 0, true);
	} else {
		v86_prev_pagedir = get_current_pagedir();
		switch_pagedir(v86_pagedir);
	}

	if (v86_thread == 0) {
		v86_thread = new_thread(v86_thread_main, (void*)1);
		if (v86_thread == 0) return false;

		v86_thread->user_ex_handler = v86_ex_handler;

		v86_retval = false;
		start_thread(v86_thread);
		while (!v86_retval) yield();
	}

	v86_alloc_addr = (void*)V86_ALLOC_ADDR;

	memset(&v86_regs, 0, sizeof(v86_regs));

	return true;
}

void v86_end_session() {
	switch_pagedir(v86_prev_pagedir);

	v86_prev_pagedir = 0;
	v86_caller_thread = 0;

	mutex_unlock(&v86_mutex);
}

void* v86_alloc(size_t size) {
	void* addr = v86_alloc_addr;

	v86_alloc_addr += size;

	return addr;
}

bool v86_bios_int(uint8_t int_no) {
	v86_caller_thread = current_thread;

	v86_int_no = int_no;

	int st = enter_critical(CL_NOSWITCH);
	resume_on(v86_thread);
	wait_on(current_thread);
	exit_critical(st);

	return v86_retval;
}

void v86_run_bios_int(uint32_t int_no) {
	switch_pagedir(v86_pagedir);

	uint16_t *ivt = (uint16_t*)0;

	v86_regs.cs = ivt[2 * int_no + 1];
	v86_regs.ip = ivt[2 * int_no];
	v86_regs.ss = ((V86_STACK_TOP - 0x10000) >> 4);
	v86_regs.sp = 0;

	v86_if = true;

	v86_asm_enter_v86(&v86_regs);
}

void v86_thread_main(void* z) {
	enter_critical(CL_NOSWITCH);

	if (z) v86_retval = true;
	wait_on(current_thread);

	v86_run_bios_int(v86_int_no);
}

void v86_exit_thread(bool status) {
	v86_retval = status;

	resume_on(v86_caller_thread);

	v86_thread_main(0);
}

bool v86_gpf_handler(registers_t *regs) {
	uint8_t* ip = (uint8_t*)V86_LIN_OF_SEG_OFF(regs->cs, regs->eip);
	uint16_t *stack = (uint16_t*)V86_LIN_OF_SEG_OFF(regs->ss, (regs->esp & 0xFFFF));
	uint32_t *stack32 = (uint32_t*)stack;
	bool is_operand32 = false; // bool is_address32 = false;

	while (true) {
		switch (ip[0]) {
			case 0x66:		// O32
				is_operand32 = true;
				ip++; regs->eip = (uint16_t)(regs->eip + 1);
				break;
			case 0x67:		// A32
				// is_address32 = true;
				ip++; regs->eip = (uint16_t)(regs->eip + 1);
				break;
			case 0x9C:		// PUSHF
				if (is_operand32) {
					regs->esp = ((regs->esp & 0xFFFF) - 4) & 0xFFFF;
					stack32--;
					*stack32 = regs->eflags & V86_VALID_FLAGS;
					if (v86_if)
						*stack32 |= EFLAGS_IF;
					else
						*stack32 &= ~EFLAGS_IF;
				} else {
					regs->esp = ((regs->esp & 0xFFFF) - 2) & 0xFFFF;
					stack--;
					*stack = regs->eflags;
					if (v86_if)
						*stack |= EFLAGS_IF;
					else
						*stack &= ~EFLAGS_IF;
				}
				regs->eip = (uint16_t)(regs->eip + 1);
				return true;
			case 0x9D:		// POPF
				if (is_operand32) {
					regs->eflags = EFLAGS_IF | EFLAGS_VM | (stack32[0] & V86_VALID_FLAGS);
					v86_if = (stack32[0] & EFLAGS_IF) != 0;
					regs->esp = ((regs->esp & 0xFFFF) + 4) & 0xFFFF;
				} else {
					regs->eflags = EFLAGS_IF | EFLAGS_VM | stack[0];
					v86_if = (stack[0] & EFLAGS_IF) != 0;
					regs->esp = ((regs->esp & 0xFFFF) + 2) & 0xFFFF;
				}
				regs->eip = (uint16_t)(regs->eip + 1);
				return true;
			case 0xCF:		// IRET
				v86_regs.ax = (uint16_t)regs->eax;
				v86_regs.bx = (uint16_t)regs->ebx;
				v86_regs.cx = (uint16_t)regs->ecx;
				v86_regs.dx = (uint16_t)regs->edx;
				v86_regs.di = (uint16_t)regs->edi;
				v86_regs.si = (uint16_t)regs->esi;

				v86_exit_thread(true);
			case 0xFA:		// CLI
				v86_if = false;
				regs->eip = (uint16_t)(regs->eip + 1);
				return true;
			case 0xFB:		// STI
				v86_if = true;
				regs->eip = (uint16_t)(regs->eip + 1);
				return true;
			default:
				return false;
		}
	}
}

void v86_ex_handler(registers_t *regs) {
	if (regs->int_no == EX_GENERAL_PROTECTION) {
		if (!v86_gpf_handler(regs)) v86_exit_thread(false);
	} else {
		v86_exit_thread(false);
	}
}

void v86_pf_handler(void* zero, registers_t *regs, void* addr) {
	dbg_printf("Unexpected V86 PF at 0x%p\n", addr);

	if (current_thread == v86_thread) {
		v86_exit_thread(false);
	} else {
		PANIC("V86 memory access exception.");
	}
}


/* vim: set ts=4 sw=4 tw=0 noet :*/
