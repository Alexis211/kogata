
OBJ = core/loader.o core/dbglog.o \
	core/gdt.o core/idt.o core/interrupt.o core/context_switch.o core/thread.o \
	core/frame.o core/paging.o core/freemem.o core/region.o core/kmalloc.o \
	core/worker.o core/prng.o \
	user/vfs.o user/nullfs.o user/process.o user/elf.o user/syscall.o user/ipc.o user/pager.o \
	dev/pci.o dev/pciide.o dev/v86.o dev/v86asm.o dev/vesa.o dev/pckbd.o \
	fs/iso9660.o

LIB = ../common/libc/libc.lib ../common/libkogata/libkogata.lib ../common/libalgo/libalgo.lib

CFLAGS = -I ./include -I ../common/include -DPRNG_INIT_ENTROPY=1$(shell date +%N)

LDFLAGS = -T linker.ld -Xlinker -Map=kernel.map

OUT = kernel.lib

include ../rules.make

kernel.bin: kernel.lib core/kmain.o core/sys.o
	$(LD) $(LDFLAGS) -o $@ $^

all: $(OUT) kernel.bin
