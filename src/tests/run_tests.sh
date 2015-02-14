#!/bin/bash

cd `dirname $0`

for FILE in */Makefile */*/Makefile; do
	TEST=`dirname $FILE`
	echo -n "Running test $TEST ... "
	if make -C $TEST run_test > $TEST/test.log 2>&1; then
		echo OK
	else
		echo FAIL
	fi
done
