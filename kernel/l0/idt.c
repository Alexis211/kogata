#include <idt.h>
#include <sys.h>
#include <string.h>
#include <dbglog.h>

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

/*	Called in idt_.asm when an exception fires (interrupt 0 to 31).
	Tries to handle the exception, panics if fails. */
void idt_isrHandler(registers_t *regs) {
	dbg_printf("/ ISR %i\n", regs->int_no);
	dbg_printf("| EAX: 0x%p   EBX: 0x%p   ECX: 0x%p   EDX: 0x%p\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
	dbg_printf("| EDI: 0x%p   ESI: 0x%p   ESP: 0x%p   EBP: 0x%p\n", regs->edi, regs->esi, regs->esp, regs->ebp);
	dbg_printf("| EIP: 0x%p   CS : 0x%p   DS : 0x%p   SS : 0x%p\n", regs->eip, regs->cs, regs->ds, regs->ss);
	dbg_printf("\\ EFl: 0x%p   I# : 0x%p   Err: 0x%p\n", regs->eflags, regs->int_no, regs->err_code);
} 

/*	Called in interrupt.asm when an IRQ fires (interrupt 32 to 47)
	Possibly wakes up a thread that was waiting, possibly calls a handler. */
void idt_irqHandler(registers_t *regs) {
	if (regs->err_code > 7) {
		outb(0xA0, 0x20);
	}
	outb(0x20, 0x20);

	dbg_printf("IRQ %i\n", regs->err_code);
	if (irq_handlers[regs->err_code] != 0) {
		irq_handlers[regs->err_code](regs);
	}
}

/* syscall handler */
void idt_syscallHandler(registers_t *regs) {
	dbg_printf("Syscall %i\n", regs->int_no);
	// do nothing...
}

/*	For internal use only. Sets up an entry of the IDT with given parameters. */
static void idt_setGate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
	idt_entries[num].base_lo = base & 0xFFFF;
	idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

	idt_entries[num].sel = sel;
	idt_entries[num].always0 = 0;
	idt_entries[num].flags = flags | 0x60;
}

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

	idt_setGate(0, (int32_t)isr0, 0x08, 0x8E);
	idt_setGate(1, (int32_t)isr1, 0x08, 0x8E);
	idt_setGate(2, (int32_t)isr2, 0x08, 0x8E);
	idt_setGate(3, (int32_t)isr3, 0x08, 0x8E);
	idt_setGate(4, (int32_t)isr4, 0x08, 0x8E);
	idt_setGate(5, (int32_t)isr5, 0x08, 0x8E);
	idt_setGate(6, (int32_t)isr6, 0x08, 0x8E);
	idt_setGate(7, (int32_t)isr7, 0x08, 0x8E);
	idt_setGate(8, (int32_t)isr8, 0x08, 0x8E);
	idt_setGate(9, (int32_t)isr9, 0x08, 0x8E);
	idt_setGate(10, (int32_t)isr10, 0x08, 0x8E);
	idt_setGate(11, (int32_t)isr11, 0x08, 0x8E);
	idt_setGate(12, (int32_t)isr12, 0x08, 0x8E);
	idt_setGate(13, (int32_t)isr13, 0x08, 0x8E);
	idt_setGate(14, (int32_t)isr14, 0x08, 0x8E);
	idt_setGate(15, (int32_t)isr15, 0x08, 0x8E);
	idt_setGate(16, (int32_t)isr16, 0x08, 0x8E);
	idt_setGate(17, (int32_t)isr17, 0x08, 0x8E);
	idt_setGate(18, (int32_t)isr18, 0x08, 0x8E);
	idt_setGate(19, (int32_t)isr19, 0x08, 0x8E);
	idt_setGate(20, (int32_t)isr20, 0x08, 0x8E);
	idt_setGate(21, (int32_t)isr21, 0x08, 0x8E);
	idt_setGate(22, (int32_t)isr22, 0x08, 0x8E);
	idt_setGate(23, (int32_t)isr23, 0x08, 0x8E);
	idt_setGate(24, (int32_t)isr24, 0x08, 0x8E);
	idt_setGate(25, (int32_t)isr25, 0x08, 0x8E);
	idt_setGate(26, (int32_t)isr26, 0x08, 0x8E);
	idt_setGate(27, (int32_t)isr27, 0x08, 0x8E);
	idt_setGate(28, (int32_t)isr28, 0x08, 0x8E);
	idt_setGate(29, (int32_t)isr29, 0x08, 0x8E);
	idt_setGate(30, (int32_t)isr30, 0x08, 0x8E);
	idt_setGate(31, (int32_t)isr31, 0x08, 0x8E);

	idt_setGate(32, (int32_t)irq0, 0x08, 0x8E);
	idt_setGate(33, (int32_t)irq1, 0x08, 0x8E);
	idt_setGate(34, (int32_t)irq2, 0x08, 0x8E);
	idt_setGate(35, (int32_t)irq3, 0x08, 0x8E);
	idt_setGate(36, (int32_t)irq4, 0x08, 0x8E);
	idt_setGate(37, (int32_t)irq5, 0x08, 0x8E);
	idt_setGate(38, (int32_t)irq6, 0x08, 0x8E);
	idt_setGate(39, (int32_t)irq7, 0x08, 0x8E);
	idt_setGate(40, (int32_t)irq8, 0x08, 0x8E);
	idt_setGate(41, (int32_t)irq9, 0x08, 0x8E);
	idt_setGate(42, (int32_t)irq10, 0x08, 0x8E);
	idt_setGate(43, (int32_t)irq11, 0x08, 0x8E);
	idt_setGate(44, (int32_t)irq12, 0x08, 0x8E);
	idt_setGate(45, (int32_t)irq13, 0x08, 0x8E);
	idt_setGate(46, (int32_t)irq14, 0x08, 0x8E);
	idt_setGate(47, (int32_t)irq15, 0x08, 0x8E);

	idt_setGate(64, (int32_t)syscall64, 0x08, 0x8E);

	idt_ptr.limit = (sizeof(idt_entry_t) * 256) - 1;
	idt_ptr.base = (uint32_t)&idt_entries;

	asm volatile ("lidt %0"::"m"(idt_ptr):"memory");
}

/*	Sets up an IRQ handler for given IRQ. */
void idt_handleIrq(int number, isr_handler_t func) {
	if (number < 16 && number >= 0) {
		irq_handlers[number] = func;
	}
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
