CC=gcc
CFLAGS=-I../../src/lib/include -std=c11

%.bf: %.c
	$(CC) $(CFLAGS) -o $<.bin $<
	./$<.bin > $@
	rm $<.bin

all: default.bf

