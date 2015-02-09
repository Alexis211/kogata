#pragma once

/*	The IDT is the system descriptor table that tells the CPU what to do when an interrupt fires.
	There are three categories of interrupts :
	- Exceptions ; eg page fault, divide by 0
	- IRQ : interrupts caused by hardware
	- System calls : when an applications asks the system to do something */

#include <config.h>

#define IRQ0  0
#define IRQ1  1
#define IRQ2  2
#define IRQ3  3
#define IRQ4  4
#define IRQ5  5
#define IRQ6  6
#define IRQ7  7
#define IRQ8  8
#define IRQ9  9
#define IRQ10 10
#define IRQ11 11
#define IRQ12 12
#define IRQ13 13
#define IRQ14 14
#define IRQ15 15

#define EX_DIVIDE_ERROR                  0         // No error code
#define EX_DEBUG                         1         // No error code
#define EX_NMI_INTERRUPT                 2         // No error code
#define EX_BREAKPOINT                    3         // No error code
#define EX_OVERFLOW                      4         // No error code
#define EX_BOUND_RANGE_EXCEDEED          5         // No error code
#define EX_INVALID_OPCODE                6         // No error code
#define EX_DEVICE_NOT_AVAILABLE          7         // No error code
#define EX_DOUBLE_FAULT                  8         // Yes (Zero)
#define EX_COPROCESSOR_SEGMENT_OVERRUN   9         // No error code
#define EX_INVALID_TSS                  10         // Yes
#define EX_SEGMENT_NOT_PRESENT          11         // Yes
#define EX_STACK_SEGMENT_FAULT          12         // Yes
#define EX_GENERAL_PROTECTION           13         // Yes
#define EX_PAGE_FAULT                   14         // Yes
#define EX_INTEL_RESERVED_1             15         // No
#define EX_FLOATING_POINT_ERROR         16         // No
#define EX_ALIGNEMENT_CHECK             17         // Yes (Zero)
#define EX_MACHINE_CHECK                18         // No
#define EX_INTEL_RESERVED_2             19         // No
#define EX_INTEL_RESERVED_3             20         // No
#define EX_INTEL_RESERVED_4             21         // No
#define EX_INTEL_RESERVED_5             22         // No
#define EX_INTEL_RESERVED_6             23         // No
#define EX_INTEL_RESERVED_7             24         // No
#define EX_INTEL_RESERVED_8             25         // No
#define EX_INTEL_RESERVED_9             26         // No
#define EX_INTEL_RESERVED_10            27         // No
#define EX_INTEL_RESERVED_11            28         // No
#define EX_INTEL_RESERVED_12            29         // No
#define EX_INTEL_RESERVED_13            30         // No
#define EX_INTEL_RESERVED_14            31         // No

#define EFLAGS_IF		(0x1 << 9)

typedef struct registers {
	uint32_t ds;                  // Data segment selector
	uint32_t edi, esi, ebp, useless_esp, ebx, edx, ecx, eax; // Pushed by pusha.
	uint32_t int_no, err_code;    // Interrupt number and error code (if applicable)
	uint32_t eip, cs, eflags, esp, ss; // Pushed by the processor automatically.
} registers_t;

typedef void (*isr_handler_t)(registers_t*);

void idt_init();

void idt_set_ex_handler(int number, isr_handler_t func);	//Set exception handler
void idt_set_irq_handler(int number, isr_handler_t func);	//Set IRQ handler

void dbg_dump_registers(registers_t*);

/* vim: set ts=4 sw=4 tw=0 noet :*/
