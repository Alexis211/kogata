#pragma once

#include <stddef.h>
#include <stdint.h>

#include <vfs.h>
#include <process.h>

/* elf_phdr_t :: p_type : program header entries types */
#define	PT_NULL             0
#define	PT_LOAD             1
#define	PT_DYNAMIC          2
#define	PT_INTERP           3
#define	PT_NOTE             4
#define	PT_SHLIB            5
#define	PT_PHDR             6
#define	PT_LOPROC  0x70000000
#define	PT_HIPROC  0x7fffffff
 
/* elf_phdr_t :: p_flags : program header entries flags */
#define PF_X	(1 << 0)
#define PF_W	(1 << 1)
#define PF_R	(1 << 2)

typedef struct {
	uint8_t e_ident[16];      /* ELF identification */
	uint16_t e_type;             /* 2 (exec file) */
	uint16_t e_machine;          /* 3 (intel architecture) */
	uint32_t e_version;          /* 1 */
	uint32_t e_entry;            /* starting point */
	uint32_t e_phoff;            /* program header table offset */
	uint32_t e_shoff;            /* section header table offset */
	uint32_t e_flags;            /* various flags */
	uint16_t e_ehsize;           /* ELF header (this) size */

	uint16_t e_phentsize;        /* program header table entry size */
	uint16_t e_phnum;            /* number of entries */

	uint16_t e_shentsize;        /* section header table entry size */
	uint16_t e_shnum;            /* number of entries */

	uint16_t e_shstrndx;         /* index of the section name string table */
} elf_ehdr_t;
 
typedef struct {
	uint32_t p_type;             /* type of segment */
	uint32_t p_offset;
	uint32_t p_vaddr;
	uint32_t p_paddr;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags;
	uint32_t p_align;
} elf_phdr_t;
 
bool is_elf(fs_handle_t *f);
proc_entry_t elf_load(fs_handle_t *f, process_t *process);	//Load an ELF to a process, return entry point

/* vim: set ts=4 sw=4 tw=0 noet :*/
