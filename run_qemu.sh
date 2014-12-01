#!/bin/sh

cd `dirname $0`

make -C kernel
qemu-system-i386 -kernel kernel/kernel.bin -serial stdio -s -S &
(sleep 0.1; gdb kernel.bin -x gdb_cmd)
