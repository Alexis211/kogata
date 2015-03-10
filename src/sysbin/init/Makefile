
OBJ = main.o

LIB = ../../lib/libkogata/libkogata.lib ../../common/libc/libc.lib ../../common/libalgo/libalgo.lib

CFLAGS = -I ./include -I ../../common/include -I ../../lib/include

LDFLAGS = -T ../linker.ld -Xlinker -Map=init.map

OUT = init.bin

include ../../rules.make
