
Wymagania.
===============
Aby zbudowaæ tranport GG potrzeba:

	- glib >= 2.0.0 (http://www.gtk.org/)
		Przetestowane z glib-2.8.3
	
	- libidn >= 0.3.0 (http://www.gnu.org/software/libidn/)
		Przetestowane z libidn-0.5.17

	- expat >= 1.95.1 (http://expat.sourceforge.net/)
		Przetestowane z expat-1.95.8

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

