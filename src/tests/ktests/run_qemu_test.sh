#!/bin/bash

(qemu-system-i386 -kernel test_kernel.bin -serial stdio -m 16 -display none & echo $! >pid) \
	| tee >(grep -m 1 "TEST-" >result; kill -INT `cat pid`) \

RESULT=`cat result`

rm result
rm pid

if [ $RESULT != '(TEST-OK)' ]; then exit 1; fi

