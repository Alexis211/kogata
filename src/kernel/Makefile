
OBJ = core/loader.o core/kmain.o core/dbglog.o core/sys.o \
	core/gdt.o core/idt.o core/interrupt.o core/context_switch.o core/thread.o \
	core/frame.o core/paging.o core/region.o core/kmalloc.o \
	user/vfs.o user/nullfs.o user/process.o user/elf.o

LIB = ../common/libc/libc.lib ../common/libkogata/libkogata.lib ../common/libalgo/libalgo.lib

CFLAGS = -I ./include -I ../common/include

LDFLAGS = -T linker.ld -Xlinker -Map=kernel.map

OUT = kernel.bin

include ../rules.make
