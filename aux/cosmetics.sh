#!/bin/sh

dir="`dirname $0`"

find . '(' -name "*.c" -o -name "*.h" ')' -a '(' '!' -path './libxode/*' -a '!' -path './intl/*' ')' \
	| xargs -n1 vim -u NONE -s "$dir/cosmetics.vim" 
stty sane
