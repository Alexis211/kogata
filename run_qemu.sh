#!/bin/sh

cd `dirname $0`

make -C kernel
qemu-system-i386 -kernel kernel/kernel.bin -serial stdio
