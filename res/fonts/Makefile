%.bf: %.c
	gcc -o $<.bin $<
	./$<.bin > $@
	rm $<.bin

all: default.bf

