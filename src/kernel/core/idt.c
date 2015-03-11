#include <idt.h>
#include <gdt.h>
#include <sys.h>
#include <string.h>
#include <dbglog.h>
#include <thread.h>
#include <sct.h>

struct idt_entry {
	uint16_t base_lo;		//Low part of address to jump to
	uint16_t sel;			//Kernel segment selector
	uint8_t always0;
	uint8_t type_attr;			//Type
	uint16_t base_hi;		//High part of address to jump to
} __attribute__((packed));
typedef struct idt_entry idt_entry_t;

struct idt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));
typedef struct idt_ptr idt_ptr_t;

#define GATE_TYPE_INTERRUPT 14		// IF is cleared on interrupt
#define GATE_TYPE_TRAP 15			// IF stays as is

#define GATE_PRESENT (1<<7)
#define GATE_DPL_SHIFT 5


void isr0();
void isr1();
void isr2();
void isr3();
void isr4();
void isr5();
void isr6();
void isr7();
void isr8();
void isr9();
void isr10();
void isr11();
void isr12();
void isr13();
void isr14();
void isr15();
void isr16();
void isr17();
void isr18();
void isr19();
void isr20();
void isr21();
void isr22();
void isr23();
void isr24();
void isr25();
void isr26();
void isr27();
void isr28();
void isr29();
void isr30();
void isr31();

void irq0();
void irq1();
void irq2();
void irq3();
void irq4();
void irq5();
void irq6();
void irq7();
void irq8();
void irq9();
void irq10();
void irq11();
void irq12();
void irq13();
void irq14();
void irq15();

void syscall64();

// ************************************************************
// Handler code

static idt_entry_t idt_entries[256];
static idt_ptr_t idt_ptr;

static isr_handler_t irq_handlers[16] = {0};
static isr_handler_t ex_handlers[32] = {0};

/*	Called in interrupt.s when an exception fires (interrupt 0 to 31) */
void idt_ex_handler(registers_t *regs) {
	if (ex_handlers[regs->int_no] != 0) {
		ex_handlers[regs->int_no](regs);
	} else {
		if (regs->eip >= K_HIGHHALF_ADDR) {
			dbg_printf("Unhandled exception in kernel code: %i\n", regs->int_no);
			dbg_dump_registers(regs);
			PANIC("Unhandled exception");
		} else {
			ASSERT(current_thread != 0 && current_thread->user_ex_handler != 0);
			current_thread->user_ex_handler(regs);
		}
	}

	// maybe exit
	if (current_thread != 0 && regs->eip < K_HIGHHALF_ADDR && current_thread->must_exit) {
		exit();
	}
} 

/*	Called in interrupt.s when an IRQ fires (interrupt 32 to 47) */
void idt_irq_handler(registers_t *regs) {
	int st = enter_critical(CL_EXCL);	// if someone tries to yield(), an assert will fail

	if (regs->err_code > 7) {
		outb(0xA0, 0x20);
	}
	outb(0x20, 0x20);

	if (regs->err_code == 0) {
		irq0_handler(regs, st);
	} else {
		/*dbg_printf("irq%d.", regs->err_code);*/

		if (irq_handlers[regs->err_code] != 0) {
			irq_handlers[regs->err_code](regs);
		}

		exit_critical(st);
	}

	// maybe exit
	if (current_thread != 0 && regs->eip < K_HIGHHALF_ADDR && current_thread->must_exit) {
		exit();
	}
}

/* Caled in interrupt.s when a syscall is called */
void idt_syscall_handler(registers_t *regs) {
	syscall_handler(regs);

	// maybe exit
	if (current_thread != 0 && regs->eip < K_HIGHHALF_ADDR && current_thread->must_exit) {
		exit();
	}
}

/*	For internal use only. Sets up an entry of the IDT with given parameters. */
static void idt_set_gate(uint8_t num, void (*fun)(), uint8_t type) {
	uint32_t base = (uint32_t)fun;

	idt_entries[num].base_lo = base & 0xFFFF;
	idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

	idt_entries[num].sel = K_CODE_SEGMENT;
	idt_entries[num].always0 = 0;
	idt_entries[num].type_attr = GATE_PRESENT
				| (3 << GATE_DPL_SHIFT)		// accessible from user mode
				| type;
}

static const struct {
	uint8_t num;
	void (*fun)();
	uint8_t type;
} gates[] = {
	// Most processor exceptions are traps and handling them
	// should be preemptible
	{ 0,	isr0,	GATE_TYPE_TRAP },
	{ 1,	isr1,	GATE_TYPE_TRAP },
	{ 2,	isr2,	GATE_TYPE_TRAP },
	{ 3,	isr3,	GATE_TYPE_TRAP },
	{ 4,	isr4,	GATE_TYPE_TRAP },
	{ 5,	isr5,	GATE_TYPE_TRAP },
	{ 6,	isr6,	GATE_TYPE_TRAP },
	{ 7,	isr7,	GATE_TYPE_TRAP },
	{ 8,	isr8,	GATE_TYPE_TRAP },
	{ 9,	isr9,	GATE_TYPE_TRAP },
	{ 10,	isr10,	GATE_TYPE_TRAP },
	{ 11,	isr11,	GATE_TYPE_TRAP },
	{ 12,	isr12,	GATE_TYPE_TRAP },
	{ 13,	isr13,	GATE_TYPE_TRAP },
	{ 14,	isr14,	GATE_TYPE_INTERRUPT },	// reenables interrupts later on
	{ 15,	isr15,	GATE_TYPE_TRAP },
	{ 16,	isr16,	GATE_TYPE_TRAP },
	{ 17,	isr17,	GATE_TYPE_TRAP },
	{ 18,	isr18,	GATE_TYPE_TRAP },
	{ 19,	isr19,	GATE_TYPE_TRAP },
	{ 20,	isr20,	GATE_TYPE_TRAP },
	{ 21,	isr21,	GATE_TYPE_TRAP },
	{ 22,	isr22,	GATE_TYPE_TRAP },
	{ 23,	isr23,	GATE_TYPE_TRAP },
	{ 24,	isr24,	GATE_TYPE_TRAP },
	{ 25,	isr25,	GATE_TYPE_TRAP },
	{ 26,	isr26,	GATE_TYPE_TRAP },
	{ 27,	isr27,	GATE_TYPE_TRAP },
	{ 28,	isr28,	GATE_TYPE_TRAP },
	{ 29,	isr29,	GATE_TYPE_TRAP },
	{ 30,	isr30,	GATE_TYPE_TRAP },
	{ 31,	isr31,	GATE_TYPE_TRAP },

	// IRQs are not preemptible ; an IRQ handler should do the bare minimum
	// (communication with the hardware), and then pass a message to a worker
	// process in order to do further processing
	{ 32,	irq0,	GATE_TYPE_INTERRUPT },
	{ 33,	irq1,	GATE_TYPE_INTERRUPT },
	{ 34,	irq2,	GATE_TYPE_INTERRUPT },
	{ 35,	irq3,	GATE_TYPE_INTERRUPT },
	{ 36,	irq4,	GATE_TYPE_INTERRUPT },
	{ 37,	irq5,	GATE_TYPE_INTERRUPT },
	{ 38,	irq6,	GATE_TYPE_INTERRUPT },
	{ 39,	irq7,	GATE_TYPE_INTERRUPT },
	{ 40,	irq8,	GATE_TYPE_INTERRUPT },
	{ 41,	irq9,	GATE_TYPE_INTERRUPT },
	{ 42,	irq10,	GATE_TYPE_INTERRUPT },
	{ 43,	irq11,	GATE_TYPE_INTERRUPT },
	{ 44,	irq12,	GATE_TYPE_INTERRUPT },
	{ 45,	irq13,	GATE_TYPE_INTERRUPT },
	{ 46,	irq14,	GATE_TYPE_INTERRUPT },
	{ 47,	irq15,	GATE_TYPE_INTERRUPT },

	// Of course, syscalls are preemptible
	{ 64,	syscall64,	GATE_TYPE_TRAP },

	{ 0, 0, 0 }
};

/*	Remaps the IRQs. Sets up the IDT. */
void idt_init() {
	memset((uint8_t*)&idt_entries, 0, sizeof(idt_entry_t) * 256);

	//Remap the IRQ table
	outb(0x20, 0x11);
	outb(0xA0, 0x11);
	outb(0x21, 0x20);
	outb(0xA1, 0x28);
	outb(0x21, 0x04);
	outb(0xA1, 0x02);
	outb(0x21, 0x01);
	outb(0xA1, 0x01);
	outb(0x21, 0x0);
	outb(0xA1, 0x0);

	for (int i = 0; gates[i].type != 0; i++) {
		idt_set_gate(gates[i].num, gates[i].fun, gates[i].type);
	}

	idt_ptr.limit = (sizeof(idt_entry_t) * 256) - 1;
	idt_ptr.base = (uint32_t)&idt_entries;

	asm volatile ("lidt %0"::"m"(idt_ptr):"memory");
	
	// Some setup calls that come later on are not preemptible,
	// so we wait until then to enable interrupts.
}

/*	Sets up an IRQ handler for given IRQ. */
void idt_set_irq_handler(int number, isr_handler_t func) {
	if (number < 16 && number >= 0) {
		irq_handlers[number] = func;
	}
}

/*	Sets up a handler for a processor exception */
void idt_set_ex_handler(int number, isr_handler_t func) {
	if (number >= 0 && number < 32) {
		ex_handlers[number] = func;
	}
}

void dbg_dump_registers(registers_t *regs) {
	dbg_printf("/ Exception %i\n", regs->int_no);
	dbg_printf("| EAX: 0x%p   EBX: 0x%p   ECX: 0x%p   EDX: 0x%p\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
	dbg_printf("| EDI: 0x%p   ESI: 0x%p   ESP: 0x%p   EBP: 0x%p\n", regs->edi, regs->esi, regs->esp, regs->ebp);
	dbg_printf("| EIP: 0x%p   CS : 0x%p   DS : 0x%p   SS : 0x%p\n", regs->eip, regs->cs, regs->ds, regs->ss);
	dbg_printf("| EFl: 0x%p   I# : 0x%p   Err: 0x%p\n", regs->eflags, regs->int_no, regs->err_code);
	dbg_printf("- Stack trace:\n");

	uint32_t ebp = regs->ebp, eip = regs->eip;
	int i = 0;
	while (ebp >= K_HIGHHALF_ADDR && eip >= K_HIGHHALF_ADDR && i++ < 10) {
		dbg_printf("| 0x%p	EIP: 0x%p\n", ebp, eip);
		uint32_t *d = (uint32_t*)ebp;
		ebp = d[0];
		eip = d[1];
	}

	dbg_printf("\\\n");
}

/* vim: set ts=4 sw=4 tw=0 noet :*/

