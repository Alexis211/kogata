LIB = ../../../kernel/kernel.lib

OBJ = kmain.o

CFLAGS = -I . -I ../../../common/include -I ../../../kernel/include -DIS_A_TEST
LDFLAGS = -T ../../../kernel/linker.ld -Xlinker -Map=test_kernel.map

OUT = test_kernel.bin

include ../../../rules.make

kmain.o: ../../../kernel/core/kmain.c test.c
	$(CC) -c $< -o $@ $(CFLAGS)

run_test: all
	../run_qemu_test.sh

