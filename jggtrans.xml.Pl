<!--
       To polska wersja przykładowego pliku konfiguracyjnego.

       Proszę pamiętać, że w XMLu obowiązującym kodowaniem jest UTF-8
-->

<jggtrans>

  <!-- Żeby to zadziałało z jabberd 1.4.x także plik jabber.xml musi być uaktualniony.
       Należy dodać następujące linie w sekcji <browse/>:

    <item category="service" type="x-gadugadu" jid="gg.localhost" name="GaduGadu Transport">
      <ns>jabber:iq:gateway</ns>
      <ns>jabber:iq:register</ns>
      <ns>jabber:iq:search</ns>
      <ns>jabber:iq:agent</ns>
      <ns>jabber:iq:browse</ns>
    </item>

  Oraz następujące, pod koniec pliku jabber.xml (poza <browse/>):

  <service id="gglinker">
    <host>gg.localhost</host>
    <accept>
      <ip>127.0.0.1</ip>
      <port>5353</port>
      <secret>secret</secret>
    </accept>
  </service>

  Oczywiście gg.localhost należy zmienić na własną nazwę serwisu (musi być w DNS),
  127.0.0.1 oraz 5353 na adres/port używany do komunikacji między jggtrans a jabberd,
  a secret na jakiś dowolny ciąg znaków.

  W przypadku użycia jabberd2 nie trzeba (i nie należy) zmieniać niczego
  w konfiguracji serwera!
  -->

  <!-- Dla jabberd 1.4:
       Ten wpis powinien zgadzać się z <service/> w sekcji <browse/> pliku jabber.xml -->
  <service jid="gg.localhost"/>

  <!-- A ten, z <service id="gglinker/> -->
  <connect>
    <ip>127.0.0.1</ip>
    <port>5353</port> <!-- Dla jabberd 2.0 zmień to na port routera (domyślnie: 5347) -->
    <secret>secret</secret>
  </connect>

  <register>
  	<!-- Tutaj zdefiniowany jest tekst informacji wyświetlanej
	     użytkownikowi podczas rejestracji.

	     Do formatowania tego tekstu można użyć tagów:
	     <p/> (nowy akapit) oraz <br/> (nowa linia).
	-->
  	<instructions>
		Aby się zarejestrować proszę wpisać
		swój numerek GG (UIN) w polu "username".<p/>
		Aby się wyrejestrować należy zostawić formularz pusty.<p/>
		Aby zmienić swoje dane w bazie GaduGadu należy wypełnić
		pole "nickname" oraz w razie potrzeby pozostałe.<p/>
		Wypełniając ten formularz zgadzasz się na wysłanie zamieszczonych
		tutaj danych do sms-express.com Sp. z o.o. i ich przetwarzanie
		przez tę spółkę. <p/>
		Na serwerze realizującym transport zachowane będzie jedynie hasło,
		jabberid oraz numer GG użytkownika a także numery osób w liście kontaktów.
	</instructions>
  </register>

 <search>
  	<!-- Tutaj zdefiniowana jest instrukcja przeszukiwania dla użytkowników. -->
 	<instructions>
	  	Proszę podać imię (first) i/lub nazwisko (last)
	  	i/lub ksywę (nick) i/lub miasto (city) szukanej osoby.<p/>
	 	Można także wyszukiwać według numeru UIN.
	</instructions>
 </search>

 <gateway>
  	<!--
	     Poniższy komunikat może być wyświetlony użytkownikowi
	     przy dodawaniu kontaktu GG.
	-->
 	<desc>
		Proszę podać numerek GaduGadu użytkownika z którym chcesz się
		skontaktować.
	</desc>
	<prompt>
		Numerek GG
	</prompt>
 </gateway>

 <!-- Tutaj definiuje się "wizytówkę" (vCard) transportu. -->
 <vCard>
     <FN>GaduGadu Transport</FN>
     <DESC>To jest bramka pomiędzy Jabberem a GaduGadu.</DESC>
     <URL>http://www.jabberstudio.org/projects/jabber-gg-transport/project/view.php</URL>
 </vCard>

 <!-- domyślne locale (język) użytkownika.
      Puste oznacza lokale systemowe a brak to brak tłumaczeń -->
 <default_locale>pl_PL</default_locale>

 <!--
      Konfiguracja logów.
      Można skonfigurować jeden logger typu "syslog" i/lub jeden "file".
      Można też nie skonfigurować żadnego logowania. -->
 <log type="syslog" facility="local0"/>
 <log type="file">/tmp/ggtrans.log</log>

 <!-- Odkomentuj to, jeśli chcesz, by do połączeń z serwerem GG było wykorzystane proxy -->
 <!--
 <proxy>
        <ip>127.0.0.1</ip>
        <port>8080</port>
 </proxy>
 -->

 <!-- Te wartości można zmieniać wedle własnych upodobań.
      Jak sesja niepotrzebnie dostaje timeout można to zwiększać
      w nieskończoność :-) -->
 <conn_timeout>60</conn_timeout>
 <ping_interval>10</ping_interval>

 <!-- Od jakiegoś czasu serwer GaduGadu nie odpowiada na pingi :-(
      Ale dajmy mu rok czasu na odpowiedź :-) -->
 <pong_timeout>31536000</pong_timeout>

 <!-- jak szybko ma transport spróbować się połączyć po rozłączeniu -->
 <reconnect>300</reconnect>

 <!-- Opóźnienie pomiędzy otrzymaniem informacji o nieobecności użytkownika,
      a wylogowaniem z GG. Służy to do poprawnej obsługi <presence type="invisible"/> -->
 <disconnect_delay>5</disconnect_delay>

 <!-- Katalog z danymi użytkowników. -->
 <!-- Proszę uważać na uprawnienie - hasła użytkowników (do GG) będą tam przechowywane -->
 <spool>/var/lib/jabber/spool/gg.localhost/</spool>

 <!-- Ile czekać po przerwaniu połączenia z serwerem jabbera, zanim jggtrans
      się zrestartuje. Wartoś ujemna oznacza, że jggtrans ma w takim przypadku
      przerwać pracę. -->
 <restart_timeout>60</restart_timeout>

 <!-- lista serwerów GG których jggtrans ma używać
 	<hub/> oznacza "użyj huba GG aby znaleźć właściwy serwer"
      	Element <server/> powinien zawierać adres serwera. Może też mieć
	ustawiony atrybut "port" zawierający numer portu na serwerze.
	Jeżeli TLS jest dostępne (obsługiwane przez libgadu) i nie jest
	ustawione tls="no", to połączenie z serwerem GG będzie szyfrowane.
	Uwaga: nie każdy serwer przyjmie połączenie TLS.
	Jggtrans będzie się próbował łączyć z podanymi serwerami w podanej
	kolejności i użyje pierwszego działającego.
	Rozsądna domyślna lista serwerów jest wkompilowana w jggtrans.
	TLS działa nienajlepiej, więc nie jest zalecane jego włączanie.
      -->
 <!--
 <servers>
        <server tls="no">217.17.41.84</server>
        <server tls="no">217.17.41.88</server>
        <server tls="no">217.17.41.86</server>
        <server tls="no">217.17.41.87</server>
        <server tls="no">217.17.41.85</server>
        <hub/>
        <server port="443">217.17.41.90</server>
 </servers>
 -->

 <!-- Gdzie zapisywać/sprawdzać plik z pid procesu ggtrans. -->
 <pidfile>/var/lib/jabber/ggtrans.pid</pidfile>

 <!-- Jabber ID osoby upoważnionej do zadań administracyjnych.
      Może być użyte wielokrotnie. -->
 <admin>god@heaven.org</admin>

 <!-- ACL pozwala określić dokładne zasady dostępu do transportu -->
 <acl>
	<!-- przykładowe wpisy: -->

	<!-- <allow who="admin@mydomain" what="iq/query?xmlns=http://jabber.org/protocol/stats"/> -->
		<!-- pozwala na odczytywanie statystyk przez admin@mydomain -->

	<!-- <deny who="*" what="iq/query?xmlns=http://jabber.org/protocol/stats"/> -->
		<!-- zabranie odczytywania statystyk wszystkim pozostałym użytkownikom -->

	<!-- <allow who="*@mydomain"/> -->
		<!-- pozwala na wykonywanie wszelki pozostały operacji użytkonikom 
			z domeny "mydomain" -->

	<!-- <deny what="iq/query?xmlns=jabber:x:register"/> --> 
		<!-- zabrania rejestracji innym użytkownikom -->

	<!-- <allow what="presence"/> --> 
		<!-- zezwala na wszystkie pakiety <presence/> -->

	<!-- <allow what="iq"/> --> 
		<!-- zezwala na wszystkie pozostałe pakiety <iq/> -->

	<!-- <allow what="message"/> --> 
		<!-- zezwala na wszystkie <message/> -->

	<!-- <deny/> --> 
		<!-- zabrania wszelkich innych pakietów (które i tak nie
			powinny się zjawić, ani nie będą obsłużone) -->
 </acl>



</jggtrans>
<!--
 vi: encoding=utf-8 syntax=xml
 -->
