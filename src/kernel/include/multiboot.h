#pragma once

#define MULTIBOOT_HEADER_MAGIC          0x1BADB002
#define MULTIBOOT_BOOTLOADER_MAGIC      0x2BADB002

struct  multiboot_header_t{
	unsigned long magic;
	unsigned long flags;
	unsigned long checksum;
	unsigned long header_addr;
	unsigned long load_addr;
	unsigned long load_end_addr;
	unsigned long bss_end_addr;
	unsigned long entry_addr;
};

struct aout_symbol_table_t {
	unsigned long tabsize;
	unsigned long strsize;
	unsigned long addr;
	unsigned long reserved;
};

struct elf_section_header_table_t {
	unsigned long num;
	unsigned long size;
	unsigned long addr;
	unsigned long shndx;
};

typedef struct {
	unsigned long flags;
	unsigned long mem_lower;
	unsigned long mem_upper;
	unsigned long boot_device;
	unsigned long cmdline;
	unsigned long mods_count;
	unsigned long mods_addr;
	union {
		struct aout_symbol_table_t aout_sym;
		struct elf_section_header_table_t elf_sec;
	};
	unsigned long mmap_length;
	unsigned long mmap_addr;
} multiboot_info_t;

typedef struct {
	unsigned long mod_start;
	unsigned long mod_end;
	unsigned long string;
	unsigned long reserved;
} multiboot_module_t;

struct memory_map_t {
	unsigned long size;
	unsigned long base_addr_low;
	unsigned long base_addr_high;
	unsigned long length_low;
	unsigned long length_high;
	unsigned long type;
};

/* vim: set ts=4 sw=4 tw=0 noet :*/
