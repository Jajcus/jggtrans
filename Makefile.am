
SUBDIRS = libxode libgadu src po

.PHONY: cosmetics ChangeLog

EXTRA_DIST = config.rpath mkinstalldirs jggtrans.xml jggtrans.xml.Pl \
	TODO BUGS README.Pl INSTALL.Pl \
	m4/codeset.m4 m4/gettext.m4 m4/glibc21.m4 m4/iconv.m4 m4/intdiv0.m4 m4/intmax.m4 \
	m4/inttypes-pri.m4 m4/inttypes.m4 m4/inttypes_h.m4 m4/isc-posix.m4 m4/lcmessage.m4 \
	m4/lib-ld.m4 m4/lib-link.m4 m4/lib-prefix.m4 m4/longdouble.m4 m4/longlong.m4 \
	m4/nls.m4 m4/po.m4 m4/printf-posix.m4 m4/progtest.m4 m4/signed.m4 m4/size_max.m4 \
	m4/stdint_h.m4 m4/uintmax_t.m4 m4/ulonglong.m4 m4/wchar_t.m4 m4/wint_t.m4 m4/xsize.m4

ChangeLog: 
	test -f .svn/entries && make cl-stamp || :
	
cl-stamp: .svn/entries
	TZ=UTC svn log -v --xml \
		| aux/svn2log.py -p '/(branches/[^/]+|trunk)' -x ChangeLog -u aux/users -F
	touch cl-stamp

cosmetics:
	./aux/cosmetics.sh

dist-hook:
	if grep '+svn' configure.ac ; then echo "Remove SVN version flag first!"; exit 1; else : ; fi

ACLOCAL_AMFLAGS = -I m4
