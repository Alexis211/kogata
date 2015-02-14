#!/bin/bash

cd `dirname $0`

for FILE in */Makefile */*/Makefile; do
	TEST=`dirname $FILE`
	echo -n "Running test $TEST ... "
	if make -C $TEST run_test > $TEST/test.log 2>&1; then
		echo -e "\033[0;32mOK\033[0m"
	else
		echo -e "\033[0;31mFAIL\033[0m"
	fi
done
