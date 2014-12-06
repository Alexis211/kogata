#!/bin/sh

cd `dirname $0`

make -C kernel || exit 1
qemu-system-i386 -kernel kernel/kernel.bin -serial stdio -m 16
