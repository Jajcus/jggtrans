#!/bin/sh

( cd libxode ; ./autogen.sh ) || exit 1

libtoolize --copy --force
aclocal 
autoheader
automake -a 
autoconf
