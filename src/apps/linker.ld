ENTRY (__libkogata_start)
 
SECTIONS{
    . = 0x100000;
 
    .text : {
        *(.text)
    }
 
    .rodata ALIGN (0x1000)  :{
        *(.rodata)
    }
 
    .data ALIGN (0x1000) : {
		start_ctors = .;
		*(.ctor*)
		end_ctors = .;
		start_dtors = .;
		*(.dtor*)
		end_dtors = .;
        *(.data)
		*(.locks)
    }
 
    .bss : {
        sbss = .;
        *(COMMON)
        *(.bss)
        ebss = .;
    }
 
	end = .; _end = .; __end = .;
}
