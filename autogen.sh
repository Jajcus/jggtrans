#!/bin/sh

( cd libxode ; ./autogen.sh ) || exit 1

libtoolize --force
rm -f po/ChangeLog
gettextize --force
aclocal -I m4
autoheader
automake -a 
autoconf
