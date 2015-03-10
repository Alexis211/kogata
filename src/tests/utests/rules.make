LIB = ../../../lib/libkogata/libkogata.lib ../../../common/libc/libc.lib

OBJ = test.o

CFLAGS = -I . -I ../../../common/include -I ../../../lib/include -DBUILD_USER_TEST
LDFLAGS = -T ../../../sysbin/linker.ld -Xlinker -Map=init.map

OUT = init.bin

include ../../../rules.make

run_test: rebuild
	../run_qemu_test.sh

