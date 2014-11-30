
AS = nasm
ASFLAGS = -felf

CC = i586-elf-gcc
CFLAGS = -ffreestanding -O2 -std=gnu99 -Wall -Wextra -I ./include
LD = i586-elf-gcc
LDFLAGS = -T linker.ld -ffreestanding -O2 -nostdlib -lgcc

OBJ = l0/loader.o l0/kmain.o

all: kernel.bin

kernel.bin: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)
