all:
	bam

clean:
	bam -c

rebuild: clean all

mrproper: clean

run_tests:
	bam
	src/tests/run_tests.sh

run_qemu: all
	qemu-system-i386 -cdrom cdrom.iso -serial stdio -m 12

run_qemu_debug: all
	qemu-system-i386 -cdrom cdrom.iso -serial stdio -m 12 -s -S &	\
	(sleep 0.1; gdb src/kernel/kernel.bin -x gdb_cmd)

run_bochs_debug: all
	bochs -f bochs_debug.cfg -q

