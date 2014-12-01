ENTRY (loader)

SECTIONS{
	k_highhalf_addr = 0xC0000000;

	. = 0x00100000;

	.setup : {
		*(.setup)
	}

	. += k_highhalf_addr;

	.text : AT(ADDR(.text) - k_highhalf_addr) {
		*(.text)
	}

	.rodata ALIGN (0x1000) : AT(ADDR(.rodata) - k_highhalf_addr) {
		*(.rodata)
	}

	.data ALIGN (0x1000) : AT(ADDR(.data) - k_highhalf_addr) {
		start_ctors = .;
		*(.ctor*)
		end_ctors = .;
		start_dtors = .;
		*(.dtor*)
		end_dtors = .;
		*(.data)
	}

	.bss : AT(ADDR(.bss) - k_highhalf_addr) {
		sbss = .;
		*(COMMON)
		*(.bss)
		ebss = .;
	}

	end = .; _end = .; __end = .;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
