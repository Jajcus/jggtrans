#!/bin/sh

find . '(' -name "*.c" -o -name "*.h" ')' -a '(' '!' -path './libxode/*' ')' \
	| xargs -n1 vim -u NONE -s cosmetics.vim 
reset
