
SUBDIRS = libxode src po

.PHONY: cosmetics

EXTRA_DIST = config.rpath mkinstalldirs jggtrans.xml jggtrans.xml.Pl TODO BUGS README.Pl INSTALL.Pl

ChangeLog: .svn/entries
	TZ=UTC svn log -v --xml | aux/svn2log.py -p '/(branches/[^/]+|trunk)' -x ChangeLog -u aux/users

cosmetics:
	./aux/cosmetics.sh
	

ACLOCAL_AMFLAGS = -I m4
