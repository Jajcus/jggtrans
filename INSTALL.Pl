
Wymagania.
===============
Aby zbudowaæ jabber-gg-transport potrzeba:

	- glib >= 1.2.0 (http://www.gtk.org/)
		Przetestowane z glib-1.2.10
	
	- libgadu >= 20030113 (czê¶æ EKG, http://dev.null.pl/ekg/)
		Prawdopodobnie potrzebny bêdzie snapshot z CVS
		Przetestowane ze snapshotem 20030113

	- pkgconfig (http://www.freedesktop.org/software/pkgconfig/)

Kompilacja
==============
Prosta jak w przypadku wiêkszo¶ci projektów OS:

	./configure
	make

Mo¿na u¿yæ opcji configure "--sysconfdir=", aby ustawiæ katalog z plikiem konfiguracyjnym,
lub "--disable-debug", aby wy³±czyæ kod "odpluskwiaj±cy".

