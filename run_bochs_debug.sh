#!/bin/sh

cd `dirname $0`

make -C kernel || exit 1
./make_cdrom.sh
bochs -f bochs_debug.cfg -q
