all:
	bam

rel:
	bam cdrom.rel.iso

clean:
	bam -c

analyze:
	scan-build -V bam

reanalyze: clean analyze

CHKFLAGS=--enable=warning,performance,information,style --suppress=portability -I src/common/include -j4 --platform=unix32
cppcheck:
	cppcheck $(CHKFLAGS) src/common 2>&1 >/dev/null | tee build/cppcheck.log
	cppcheck $(CHKFLAGS) -I src/kernel/include src/kernel 2>&1 >/dev/null | tee -a build/cppcheck.log
	cppcheck $(CHKFLAGS) -I src/lib/include src/lib src/sysbin 2>&1 >/dev/null | tee -a build/cppcheck.log

rebuild: clean all

mrproper: clean

run_tests:
	rm build/dev/tests/*.log build/dev/tests/*.log.err || true
	bam test.dev

run_qemu: all
	qemu-system-i386 -cdrom cdrom.dev.iso -serial stdio -m 12 </dev/null

run_qemu_rel: rel
	qemu-system-i386 -cdrom cdrom.rel.iso -serial stdio -m 12 </dev/null

run_qemu_debug: all
	qemu-system-i386 -cdrom cdrom.dev.iso -serial stdio -m 12 -s -S </dev/null &	\
	(sleep 0.1; gdb src/kernel/kernel.bin -x gdb_cmd)

run_bochs_debug: all
	bochs -f bochs_debug.cfg -q

