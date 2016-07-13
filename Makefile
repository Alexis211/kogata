all:
	bam

clean:
	bam -c

rebuild: clean all

mrproper: clean

run_tests:
	rm build/tests/*.log build/tests/*.log.err || true
	bam test

run_qemu: all
	qemu-system-i386 -cdrom cdrom.iso -serial stdio -m 12 </dev/null

run_qemu_debug: all
	qemu-system-i386 -cdrom cdrom.iso -serial stdio -m 12 -s -S </dev/null &	\
	(sleep 0.1; gdb src/kernel/kernel.bin -x gdb_cmd)

run_bochs_debug: all
	bochs -f bochs_debug.cfg -q

