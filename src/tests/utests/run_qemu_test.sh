#!/bin/bash

if [ $1 = 'watchdog' ]; then
	sleep 10 &
	PID=$!
	echo $PID > pid2
	wait $PID
	if [ $? -eq 0 ]; then echo "(TEST-FAIL)"; fi
	exit 0
fi

(qemu-system-i386 -kernel ../../../kernel/kernel.bin  -append 'init=io:/mod/init.bin' -initrd init.bin -serial stdio -m 16 -display none & echo $! >pid &
 $0 watchdog) \
	| tee >(grep -m 1 "TEST-" >result; kill -INT `cat pid`; kill -TERM `cat pid2`) \

RESULT=`cat result`

rm result
rm pid
rm pid2

if [ $RESULT != '(TEST-OK)' ]; then exit 1; fi

