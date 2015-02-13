#!/bin/sh

cd `dirname $0`

make || exit 1
qemu-system-i386 -kernel src/kernel/kernel.bin -serial stdio -s -S &
(sleep 0.1; gdb src/kernel/kernel.bin -x gdb_cmd)
