CC=gcc
CFLAGS=-I../../src/lib/include/proto -std=c11

all: fr.km

%.km: %.c
	$(CC) $(CFLAGS) -o $<.bin $<
	./$<.bin > $@
	rm $<.bin

clean:
	rm *.km || exit 0

rebuild: clean all

