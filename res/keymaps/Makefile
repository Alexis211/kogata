CC=gcc
CFLAGS=-I../../src/lib/include -std=c11

%.km: %.c
	$(CC) $(CFLAGS) -o $<.bin $<
	./$<.bin > $@
	rm $<.bin

all: fr.km

