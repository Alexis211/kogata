
AS = nasm
ASFLAGS = -felf -g

CC = i586-elf-gcc
CFLAGS = -ffreestanding -O2 -std=gnu99 -Wall -Wextra -I . -I ./include -g
# CXX = i586-elf-g++
# CXFLAGS = -ffreestanding -O3 -Wall -Wextra -I . -I ./include -fno-exceptions -fno-rtti
LD = i586-elf-gcc
LDFLAGS = -T linker.ld -ffreestanding -O2 -nostdlib -lgcc

OBJ = lib/string.o lib/printf.o \
	l0/loader.o l0/kmain.o l0/dbglog.o l0/sys.o \
	l0/gdt.o l0/idt.o l0/interrupt.o
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
	rm $(OUT)

rebuild: mrproper all
