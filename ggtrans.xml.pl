<!--
       To polska wersja przykładowego pliku konfiguracyjnego.

       Proszę pamiętać, że w XMLu obowiązującym kodowaniem jest UTF-8
-->

<!--run: /home/jacek/CVS/bnet/src/jabber-gg-transport/src/jggtrans -->

<ggtrans>

  <!-- Żeby to zadziałało także plik jabber.xml musi być uaktualniony.
       Należy dodać następujące linie w sekcji <browse/>:
    
    <service type="gg" jid="gg.localhost" name="GaduGadu Transport">
      <ns>jabber:iq:gateway</ns>
      <ns>jabber:iq:register</ns>
      <ns>jabber:iq:search</ns>
    </service>

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
  -->
   
  <!-- Ten wpis powinien zgadzać się z <service/> w sekcji <browse/> pliku jabber.xml -->
  <service type="gg" jid="gg.localhost" name="GaduGadu Transport"/>

  <!-- A ten, z <service id="gglinker/> --> 
  <connect id="gglinker">
    <ip>127.0.0.1</ip>
    <port>5353</port>
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
		rolę "e-mail" i "nickname" oraz w razie potrzeby pozostałe.
	</instructions>
  </register>
  
 <search>
  	<!-- Tutaj zdefiniowana jest instrukcja przeszukiwania dla użytkowników. -->
 	<instructions>
	  	Proszę podać imię (first) i/lub nazwisko (last) 
	  	i/lub ksywę (nick) i/lub miasto (city) szukanej osoby.<p/>
	 	Można także wyszukiwać według adresu e-mail, lub numeru UIN.
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
     <DESC>To jest bramka pomiędzy Jabberem, a GaduGadu.</DESC>
     <URL>http://foo.bar/</URL>
 </vCard>

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

 <!-- Katalog z danymi użytkowników. -->
 <!-- Proszę uważać na uprawnienie - hasła użytkowników (do GG) będą tam przechowywane -->
 <spool>/var/lib/jabber/spool/gg.localhost/</spool>

 <!-- Gdzie zapisywać/sprawdzać plik z pid procesu ggtrans. -->
 <pidfile>/var/lib/jabber/ggtrans.pid</pidfile>

</ggtrans>
<!--
 vi: encoding=utf-8 syntax=xml
 -->
