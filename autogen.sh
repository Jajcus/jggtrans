#!/bin/sh

( cd libxode ; ./autogen.sh ) || exit 1

libtoolize --force
rm -f po/ChangeLog
touch po/Makefile.in m4/Makefile
gettextize --force
aclocal -I m4
autoheader
automake -a 
autoconf
