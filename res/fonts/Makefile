CC=gcc
CFLAGS=-I../../src/lib/include -std=c11
AS=nasm
ASFLAGS=-fbin

all: pcvga.bf muazzam.bf

%.bf: %.c
	$(CC) $(CFLAGS) -o $<.bin $<
	./$<.bin > $@
	rm $<.bin

%.bf: %.s
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm *.bf || exit 0

rebuild: clean all
