
SUBDIRS = m4 libxode src po

.PHONY: cosmetics ChangeLog

EXTRA_DIST = config.rpath mkinstalldirs jggtrans.xml jggtrans.xml.Pl TODO BUGS README.Pl INSTALL.Pl

ChangeLog: 
	TZ=UTC svn log -v --xml | svn2log.py -p '/(branches/[^/]+|trunk)' -x ChangeLog -u aux/users

cosmetics:
	./aux/cosmetics.sh
	

ACLOCAL_AMFLAGS = -I m4
