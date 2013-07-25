
SUBDIRS = libxode src po

.PHONY: cosmetics ChangeLog

EXTRA_DIST = config.rpath  config.rpath jggtrans.xml jggtrans.xml.Pl \
	README.Pl INSTALL.Pl

ChangeLog: 
	test -d .git && make cl-stamp || :
	
cl-stamp: .git
	git log > ChangeLog
	touch cl-stamp

cosmetics:
	./aux/cosmetics.sh

dist-hook:
	if grep '+git' configure.ac ; then echo "Remove SVN version flag first!"; exit 1; else : ; fi

ACLOCAL_AMFLAGS = -I m4
