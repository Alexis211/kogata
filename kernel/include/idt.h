#pragma once

/*	The IDT is the system descriptor table that tells the CPU what to do when an interrupt fires.
	There are three categories of interrupts :
	- Exceptions ; eg page fault, divide by 0
	- IRQ : interrupts caused by hardware
	- System calls : when an applications asks the system to do something */

#include <config.h>

struct idt_entry {
	uint16_t base_lo;		//Low part of address to jump to
	uint16_t sel;			//Kernel segment selector
	uint8_t always0;
	uint8_t flags;			//Flags
	uint16_t base_hi;		//High part of address to jump to
} __attribute__((packed));
typedef struct idt_entry idt_entry_t;

struct idt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));
typedef struct idt_ptr idt_ptr_t;

struct registers {
	uint32_t ds;                  // Data segment selector
	uint32_t edi, esi, ebp, useless_esp, ebx, edx, ecx, eax; // Pushed by pusha.
	uint32_t int_no, err_code;    // Interrupt number and error code (if applicable)
	uint32_t eip, cs, eflags, esp, ss; // Pushed by the processor automatically.
};
typedef struct registers registers_t;

typedef void (*isr_handler_t)(registers_t*);

void idt_init();
//void idt_handleIrq(int number, isr_handler_t func);	//Set IRQ handler


/* vim: set ts=4 sw=4 tw=0 noet :*/
