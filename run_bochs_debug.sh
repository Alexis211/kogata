#!/bin/sh

cd `dirname $0`

make || exit 1
./make_cdrom.sh
bochs -f bochs_debug.cfg -q
