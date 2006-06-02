
		     GaduGadu Transport for Jabber
		    ===============================
	    (c) 2002-2006 Jacek Konieczny [jajcus(a)jajcus,net]

Jest to zewnêtrzny komponent dla serwera Jabber (http://www.jabber.org)
realizuj±cy bramkê do serwisu GaduGadu.

Pakiet zawiera przyk³adowe pliki konfiguracyjne w dwóch wersjach
jêzykowych: 
	jggtrans.xml    ---  angielska
	jggtrans.xml.Pl ---  polska

Jeden z tych plików powinien byæ zainstalowany jako "ggtrans.xml"
w katalogu podanym opcj± "--sysconfdir=..." do configure 
(domy¶lnie: /usr/local/etc).
	
Pliki konfiguracyjne zawieraj± komentarze, które powinny wystarczyæ do
prawid³owego skonfigurowania programu.

Po skonfigurowaniu transport mo¿na uruchomiæ startuj±c "jggtrans"
po odpowiednim zmodyfikowaniu jabber.xml i uruchomieniu serwera Jabber.

jggtrans mo¿e byæ uruchomiony z konta "root", ale tylko gdy podana
bêdzie opcja "-u" (oraz opcjonalnie "-g"). W takim przypadku program
zmieni uid i gid zaraz po starcie.

Wszelkie b³êdy oraz pro¶by o ulepszenia najlepiej umieszczaæ na stronie
projektu: http://jggtrans.jajcus.net/

Mo¿na oczywi¶cie te¿ s³aæ mailem do mnie.

Jest te¿ lista dyskusyjna: jggtrans@lists.jajcus.net
Aby siê na ni± zapisaæ nale¿y wys³aæ majla na adres: jggtrans-subscribe@lists.jajcus.net


	Mi³ego Jabberowania :-)
	
		Jacek Konieczny
		

# vi: tw=72 fo=tcql1 
