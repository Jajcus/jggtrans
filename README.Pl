
		     GaduGadu Transport for Jabber
		    ===============================
	    (c) 2002-2010 Jacek Konieczny [jajcus(a)jajcus,net]

Jest to zewn�trzny komponent dla serwera Jabber (http://www.jabber.org)
realizuj�cy bramk� do serwisu GaduGadu.

Pakiet zawiera przyk�adowe pliki konfiguracyjne w dw�ch wersjach
j�zykowych: 
	jggtrans.xml    ---  angielska
	jggtrans.xml.Pl ---  polska

Jeden z tych plik�w powinien by� zainstalowany jako "ggtrans.xml"
w katalogu podanym opcj� "--sysconfdir=..." do configure 
(domy�lnie: /usr/local/etc).
	
Pliki konfiguracyjne zawieraj� komentarze, kt�re powinny wystarczy� do
prawid�owego skonfigurowania programu.

Po skonfigurowaniu transport mo�na uruchomi� startuj�c "jggtrans"
po odpowiednim zmodyfikowaniu jabber.xml i uruchomieniu serwera Jabber.

jggtrans mo�e by� uruchomiony z konta "root", ale tylko gdy podana
b�dzie opcja "-u" (oraz opcjonalnie "-g"). W takim przypadku program
zmieni uid i gid zaraz po starcie.

Wszelkie b��dy oraz pro�by o ulepszenia najlepiej umieszcza� na stronie
projektu: http://jggtrans.jajcus.net/

Mo�na oczywi�cie te� s�a� mailem do mnie.

Jest te� lista dyskusyjna: jggtrans@lists.jajcus.net
Aby si� na ni� zapisa� nale�y wys�a� majla na adres: jggtrans-subscribe@lists.jajcus.net


	Mi�ego Jabberowania :-)
	
		Jacek Konieczny
		

# vi: tw=72 fo=tcql1 
