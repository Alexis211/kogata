#include <gdt.h>
#include <string.h>

#define GDT_ENTRIES 6	// The contents of each entry is defined in gdt_init.

static gdt_entry_t gdt_entries[GDT_ENTRIES];
static gdt_ptr_t gdt_ptr;

/*	For internal use only. Writes one entry of the GDT with given parameters. */
static void gdt_setGate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
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

	gdt_setGate(0, 0, 0, 0, 0);					//Null segment
	gdt_setGate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);	//Kernel code segment	0x08
	gdt_setGate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);	//Kernel data segment	0x10
	gdt_setGate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);	//User code segment		0x18
	gdt_setGate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);	//User data segment		0x20

	asm volatile ("lgdt %0"::"m"(gdt_ptr):"memory");
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
