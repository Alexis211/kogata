#!/bin/sh

gdb -batch -ex "file $1" -ex "disass /m $2"
