#include <string.h>

#include <elf.h>


bool is_elf(fs_handle_t *f) {
	elf_ehdr_t h;
	if (file_read(f, 0, sizeof(elf_ehdr_t), (char*)&h) != sizeof(elf_ehdr_t)) return false;

	return (h.e_ident[0] == 0x7F && h.e_ident[1] == 'E'
		  && h.e_ident[2] == 'L' && h.e_ident[3] == 'F');
}

proc_entry_t elf_load(fs_handle_t *f, process_t* process) {
	if (!is_elf(f)) return 0;

	elf_ehdr_t ehdr;
	if (file_read(f, 0, sizeof(elf_ehdr_t), (char*)&ehdr) != sizeof(elf_ehdr_t)) return 0;

	elf_phdr_t phdr;
	int i;

	pagedir_t *r = get_current_pagedir();
	switch_pagedir(proc_pagedir(process));

	// TODO : when we fail, free ressources ?

	for (i = 0; i < ehdr.e_phnum; i++) {
		size_t read_phdr_r = 
			file_read(f,
					ehdr.e_phoff + i * sizeof(elf_phdr_t),
					sizeof(elf_phdr_t),
					(char*)&phdr);
		if (read_phdr_r != sizeof(elf_phdr_t)) goto error;

		if (phdr.p_type == PT_LOAD) {
			if (phdr.p_flags & PF_W) {
				mmap(process, (void*)phdr.p_vaddr, phdr.p_memsz,
						((phdr.p_flags & PF_R) ? MM_READ : 0) | MM_WRITE);

				size_t read_r = file_read(f, phdr.p_offset, phdr.p_filesz, (char*)phdr.p_vaddr);
				if (read_r != phdr.p_filesz) goto error;

				if (phdr.p_memsz > phdr.p_filesz) {
					memset((char*)phdr.p_vaddr + phdr.p_filesz, 0, phdr.p_memsz - phdr.p_filesz);
				}
			} else {
				if (phdr.p_filesz != phdr.p_memsz) {
					dbg_printf("Strange ELF file...\n");
					goto error;
				}

				mmap_file(process,
						f, phdr.p_offset,
						(void*)phdr.p_vaddr, phdr.p_memsz,
						((phdr.p_flags & PF_R) ? MM_READ : 0) | ((phdr.p_flags & PF_X) ? MM_EXEC : 0));
			}
		}
	}

	switch_pagedir(r);

	return (proc_entry_t)ehdr.e_entry;

error:
	switch_pagedir(r);
	return 0;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
