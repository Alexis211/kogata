#!/bin/bash

BINFILE=$1
LOGFILE=$2
KERNEL=$3

RESULTFILE=`mktemp`
PIDFILE=`mktemp`

(timeout 5s qemu-system-i386 -kernel $KERNEL -append "init=io:/mod/`basename $BINFILE`" \
							 -initrd "$BINFILE" -serial stdio -m 16 -display none 2>/dev/null \
	& echo $! >$PIDFILE) \
	| tee >(grep -m 1 "TEST-" >$RESULTFILE; kill -INT `cat $PIDFILE`) >$LOGFILE

RESULT=`cat $RESULTFILE`

rm $RESULTFILE
rm $PIDFILE

if [ "$RESULT" != '[1] (TEST-OK)' ]; then
	echo -e "\033[0;31m$BINFILE $RESULT\033[0m"
	cp $LOGFILE $LOGFILE.err
	exit 1;
else
	echo -e "\033[0;32m$BINFILE $RESULT\033[0m"
fi

