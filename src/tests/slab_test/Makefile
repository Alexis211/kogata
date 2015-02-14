slab_test.bin: slab_test.c ../../common/include/slab_alloc.h ../../common/libkogata/slab_alloc.c
	gcc -m32 -o $@ -std=c99 -I ../../common/include slab_test.c ../../common/libkogata/slab_alloc.c

run_test: slab_test.bin
	./slab_test.bin
