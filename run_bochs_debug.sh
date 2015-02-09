#!/bin/sh

cd `dirname $0`

make -C src/common || exit 1
make -C src/kernel || exit 1
./make_cdrom.sh
bochs -f bochs_debug.cfg -q
