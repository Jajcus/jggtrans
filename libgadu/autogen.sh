#!/bin/sh

libtoolize --force || exit $?
aclocal -I m4 || exit $?
autoheader || exit $?
automake --foreign -a || exit $?
autoconf || exit $?
