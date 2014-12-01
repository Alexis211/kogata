#!/bin/sh

cd `dirname $0`

make -C kernel
./make_cdrom.sh
bochs -f bochs_debug.cfg -q
