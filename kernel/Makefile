
AS = nasm
ASFLAGS = -felf -g

CC = i586-elf-gcc
CFLAGS = -ffreestanding -O2 -std=gnu99 -Wall -Wextra -I . -I ./include -g -Wno-unused-parameter
# CXX = i586-elf-g++
# CXFLAGS = -ffreestanding -O3 -Wall -Wextra -I . -I ./include -fno-exceptions -fno-rtti
LD = i586-elf-gcc
LDFLAGS = -T linker.ld -ffreestanding -O2 -nostdlib -lgcc -Xlinker -Map=kernel.map

OBJ = lib/string.o lib/printf.o lib/slab_alloc.o lib/mutex.o lib/hashtbl.o \
	l0/loader.o l0/kmain.o l0/dbglog.o l0/sys.o \
	l0/gdt.o l0/idt.o l0/interrupt.o l0/context_switch.o l0/thread.o \
	l0/frame.o l0/paging.o l0/region.o l0/kmalloc.o
OUT = kernel.bin

all: $(OUT)

$(OUT): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

# %.o: %.cpp
#	$(CXX) -c $< -o $@ $(CXFLAGS)

clean:
	rm */*.o || true
mrproper: clean
	rm $(OUT) || true

rebuild: mrproper all
