#include <gdt.h>
#include <string.h>

#define GDT_ENTRIES 6	// The contents of each entry is defined in gdt_init.

/* 	One entry of the table */
typedef struct {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_middle;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

/*	Structure defining the whole table : address and size (in bytes). */
typedef struct {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

/*	The TSS is used for hardware multitasking.
	We don't use that, but we still need a TSS so that user mode process exceptions
	can be handled correctly by the kernel.	*/
typedef struct {
	uint32_t prev_tss;   // The previous TSS - if we used hardware task switching this would form a linked list.
	uint32_t esp0;       // The stack pointer to load when we change to kernel mode.
	uint32_t ss0;        // The stack segment to load when we change to kernel mode.
	uint32_t esp1;       // Unused...
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;         // The value to load into ES when we change to kernel mode.
	uint32_t cs;         // The value to load into CS when we change to kernel mode.
	uint32_t ss;         // The value to load into SS when we change to kernel mode.
	uint32_t ds;         // The value to load into DS when we change to kernel mode.
	uint32_t fs;         // The value to load into FS when we change to kernel mode.
	uint32_t gs;         // The value to load into GS when we change to kernel mode.
	uint32_t ldt;        // Unused...
	uint16_t trap;
	uint16_t iomap_base;
	uint8_t iomap[8192];
} __attribute__((packed)) tss_entry_t;

// ========================= //
// Actual definitions

static gdt_entry_t gdt_entries[GDT_ENTRIES];
static gdt_ptr_t gdt_ptr;
static tss_entry_t tss_entry;

/*	For internal use only. Writes one entry of the GDT with given parameters. */
static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
	gdt_entries[num].base_low = (base & 0xFFFF);
	gdt_entries[num].base_middle = (base >> 16) & 0xFF;
	gdt_entries[num].base_high = (base >> 24) & 0xFF;

	gdt_entries[num].limit_low = (limit & 0xFFFF);
	gdt_entries[num].granularity = (limit >> 16) & 0x0F;
	gdt_entries[num].granularity |= gran & 0xF0;
	gdt_entries[num].access = access;
}

/*	Write data to the GDT and enable it. */
void gdt_init() {
	gdt_ptr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
	gdt_ptr.base = (uint32_t)&gdt_entries;

	gdt_set_gate(0, 0, 0, 0, 0);					//Null segment
	gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);		//Kernel code segment	0x08
	gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);		//Kernel data segment	0x10
	gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);		//User code segment		0x18
	gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);		//User data segment		0x20

	// Write TSS
	memset(&tss_entry, 0, sizeof(tss_entry_t));

	tss_entry.ss0  = 0x10;
	tss_entry.esp0 = 0;
	tss_entry.iomap_base = sizeof(tss_entry_t) - 8192;

	uint32_t tss_base = (uint32_t)&tss_entry;
	uint32_t tss_limit = tss_base + sizeof(tss_entry_t);

	gdt_set_gate(5, tss_base, tss_limit, 0xE9, 0x00);

	asm volatile("lgdt %0"::"m"(gdt_ptr):"memory");

	asm volatile("movw $0x2b, %%ax; ltr %%ax":::"%eax");
}

void set_kernel_stack(void* addr) {
	tss_entry.esp0 = (uint32_t)addr;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
