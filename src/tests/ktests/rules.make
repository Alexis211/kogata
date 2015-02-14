LIB = ../../../kernel/kernel.lib

OBJ = kmain.o sys.o

CFLAGS = -I . -I ../../../common/include -I ../../../kernel/include -DBUILD_KERNEL_TEST
LDFLAGS = -T ../../../kernel/linker.ld -Xlinker -Map=test_kernel.map

OUT = test_kernel.bin

include ../../../rules.make

%.o: ../../../kernel/core/%.c test.c
	$(CC) -c $< -o $@ $(CFLAGS)

run_test: rebuild
	../run_qemu_test.sh

