#!/bin/bash

BINFILE=$1
LOGFILE=$2
MAPFILE=$3

RESULTFILE=`mktemp`
PIDFILE=`mktemp`

(timeout 3s qemu-system-i386 -kernel $BINFILE -initrd $MAPFILE -serial stdio -m 16 -display none 2>/dev/null \
		& echo $! >$PIDFILE) \
	| tee >(grep -m 1 "TEST-" >$RESULTFILE; kill -INT `cat $PIDFILE`) > $LOGFILE

RESULT=`cat $RESULTFILE`

rm $RESULTFILE
rm $PIDFILE

if [ "$RESULT" != '(TEST-OK)' ]; then
	echo -e "\033[0;31m$BINFILE $RESULT\033[0m"
	cp $LOGFILE $LOGFILE.err
	exit 1;
else
	echo -e "\033[0;32m$BINFILE $RESULT\033[0m"
fi

