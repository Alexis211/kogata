#!/bin/sh

cd `dirname $0`

make || exit 1
qemu-system-i386 -kernel src/kernel/kernel.bin -serial stdio -m 16
