
Wymagania.
===============
Aby zbudowaæ jabber-gg-transport potrzeba:

	- glib >= 1.2.0 (http://www.gtk.org/)
		Przetestowane z glib-1.2.10
	
	- libgadu >= 1.4 (czê¶æ EKG, http://dev.null.pl/ekg/)
		Przetestowane z libgadu-1.4
		libgadu-1.4 skompilowane z obs³ug± w±tków (pthread) czasami
		tworzy bardzo du¿o niepotrzebnych w±tków. Zaleca siê
		stosowanie libgadu skompilowanego bez obs³ugi pthread.

	- pkgconfig (http://www.freedesktop.org/software/pkgconfig/)

Kompilacja
==============
Prosta jak w przypadku wiêkszo¶ci projektów OS:

	./configure
	make

Mo¿na u¿yæ opcji configure "--sysconfdir=", aby ustawiæ katalog z plikiem konfiguracyjnym,
lub "--disable-debug", aby wy³±czyæ kod "odpluskwiaj±cy".

Na FreeBSD i w innych systemach mo¿e okazaæ siê konieczne takie wywo³anie configure:
	
	./configure CPPFLAGS="-I/usr/local/include" LDFLAGS="-L/usr/local/lib"

inaczej skrypt configure mo¿e poprawnie nie wykryæ zainstalowanych w systemie
bibliotek, takich jak gettext.

Je¶li naprawdê chcesz skompilowaæ jggtrans ze snapshotem libgadu u¿yj:

	./configure --with-libgadu-snapshot

