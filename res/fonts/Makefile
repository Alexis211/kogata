CC=gcc
CFLAGS=-I../../src/lib/include -std=c11
AS=nasm
ASFLAGS=-fbin

%.bf: %.c
	$(CC) $(CFLAGS) -o $<.bin $<
	./$<.bin > $@
	rm $<.bin

%.bf: %.s
	$(AS) $(ASFLAGS) -o $@ $<

all: pcvga.bf muazzam.bf

clean:
	rm *.bf || exit 0

