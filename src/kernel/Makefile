
OBJ = core/loader.o core/kmain.o core/dbglog.o core/sys.o \
	core/gdt.o core/idt.o core/interrupt.o core/context_switch.o core/thread.o \
	core/frame.o core/paging.o core/region.o core/kmalloc.o

LIB = ../common/common.lib

CFLAGS = -I ./include -I ../common/include

LDFLAGS = -T linker.ld -Xlinker -Map=kernel.map

OUT = kernel.bin

include ../rules.make
