#!/bin/sh

( cd libxode ; ./autogen.sh ) || exit 1

libtoolize --force
rm -f po/ChangeLog
gettextize --force
aclocal
autoheader
automake -a 
autoconf
