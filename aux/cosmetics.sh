#!/bin/sh

find . '(' -name "*.c" -o -name "*.h" ')' -a '(' '!' -path './libxode/*' -a '!' -path './intl/*' ')' \
	| xargs -n1 vim -u NONE -s cosmetics.vim 
stty sane
