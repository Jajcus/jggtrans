#!/bin/sh

libtoolize --copy --force
aclocal 
autoheader
automake -a 
autoconf
