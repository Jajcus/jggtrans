#!/bin/sh

( cd libxode ; ./autogen.sh ) || exit 1

libtoolize --force || exit $?
rm -f po/ChangeLog
autopoint --force || exit $?
touch po/Makefile.in m4/Makefile
aclocal -I m4 || exit $?
autoheader || exit $?
automake -a || exit $?
autoconf || exit $?
