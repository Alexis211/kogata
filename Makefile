DIRS = src/common/libkogata src/common/libc src/common/libalgo src/kernel src/lib/libkogata src/apps/init

all:
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1;   \
	done

rebuild:
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir rebuild || exit 1;   \
	done

clean:
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean;   \
	done

mrproper:
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir mrproper;   \
	done

run_tests: rebuild
	src/tests/run_tests.sh

run_qemu: all
	qemu-system-i386 -kernel src/kernel/kernel.bin -serial stdio -m 16 -initrd src/apps/init/init.bin

run_qemu_debug: all
	qemu-system-i386 -kernel src/kernel/kernel.bin -serial stdio -s -S &	\
	(sleep 0.1; gdb src/kernel/kernel.bin -x gdb_cmd)

run_bochs_debug: all
	./make_cdrom.sh
	bochs -f bochs_debug.cfg -q

