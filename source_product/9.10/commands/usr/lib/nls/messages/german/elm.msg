$set 1
1 (Alias "%s" ist unbekannt)\n\r
2 Alias '%s' kann nicht erweitert werden!\n\r
3 Gœltige Adressen mit "elmalias -c" suchen!\n\r
4 '%s' ist eine unbekannte Adresse. Ersetzen durch: 
5 Rœcksprungadresse kann nicht erstellt werden - MAX_HOPS-Limit erreicht!
$set 2
1 Alias-Namen eingeben: 
2 Gleichnamige Gruppe existiert bereits: %s
3 Ein Alias hierfœr existiert bereits: %s
4 Voller Name fœr %s: 
5 Adresse fœr %s eingeben: 
6 Keine Adresse angegeben!
7 Keine Nachricht fœr ein Alias vorhanden!
8 Alias fœr die Adresse der aktuellen Nachricht: 
11 Voller Name von %s: 
12 %s konnte nicht geŒffnet werden, um neues Alias anzufœgen!
13 Neues Alias konnte nicht an Datei %s angefœgt werden
14 Alias-Befehle
15 A=Alias fœr akt. Nachr.  M=Alias erstellen  D=Alias lŒschen  P=Namen prœfen
16 E=Alias erweitern u. prœfen  U=Benutzer-Alias-Namen  S=System-Alias-Namen
17 Interne Tabellen werden initialisiert...
18 Alias: 
21 Suchen nach folgendem Namen: 
22 Gruppen-Alias:\n\r%s
23 Alias-Adresse:\n\r%s
24 nicht gefunden
25 Liste der System-Alias-Namen:\r\n
26 \n\n(Bildschirminhalt kann bei Bedarf mit ^L wiederhergestellt werden)\n
27 \n\nZurœck zum Alias-Hauptmenœ durch Drœcken einer beliebigen Taste:
28 Systemdatei %s nicht lesbar
29 Liste der Benutzer-Alias-Namen:\r\n
32 %s-Benutzerdatei nicht lesbar
38 Ungœltige Eingabe!
39 Alias wird gelŒscht...
40 Neue Alias-Namen werden hinzugefœgt...
41 Datenbank wird erneut eingelesen...
42 Neue Alias-Namen erfolgreich installiert
43 Alias-Namen erfolgreich gelŒscht
44 'elmalias' fehlgeschlagen.  Bitte 'aliases.text' bzw. 'elmalias' prœfen.
45 Taste, fœr die Hilfe gewœnscht wird : 
46 Taste, fœr die Hilfe gewœnscht wird, '?' fœr Liste oder '.' zum Verlassen
47 Tasten: 
48 a = Rœcksendeadresse der aktuellen Nachricht in Alias-Datenbank eintragen
49 m = Neuen Benutzer-Alias generieren, in Alias-Datenbank eintragen
50 Zurœck zum Hauptmenœ.
51 p = Nach Person in Alias-Datenbank suchen
52 s = System-Alias-Datenbank anzeigen
53 Taste wird hier nicht benutzt.
56 Hash-Datei %s geŒffnet, aber Datendatei konnte nicht geŒffnet werden!\n
57 d = Benutzer-Alias lŒschen und anschlieﬁend aus Alias-Datenbank lŒschen
58 u = Benutzer-Alias-Datenbank anzeigen
59 e = Nach Alias mit vollstÃndig angegebener Adresse suchen
60 ^L = Bildschirm neu aufbauen.
61 Alias, das auf volle LÃnge erweitert werden soll: 
62 Alias-Adresse:\n\r%s
63 Drœcken Sie eine beliebige Taste, um weiterzumachen: 
$set 3
1 Interne Tabellen werden initialisiert...
$set 4
1 %s ist eine ungœltige Adresse!
2 \n** Ihre System-Alias-Datei wurde offenbar fœr eine andere Version des\n
3 ** Elm-Systems erstellt. Sie muﬁ vom Systemverwalter als 'root' mit dem\n
4 ** Befehl ``elmalias'' erstellt werden (keine ÿnderungen an der Datei\n
5 ** erforderlich, einfach `elmalias -q' als 'root' eingeben).\n\n
6 \n** Ihre System-Alias-Datei wurde offenbar fœr eine andere Version des\n
7 ** Elm-Systems erstellt. Sie muﬁ mit dem Befehl ``elmalias'' manuell\n
8 ** oder automatisch neu erstellt werden. Automatisch erstellen (y/n) ? 
9 \n\n** Keine automatische Ausfœhrung. Manuelle Eingabe erforderlich:\n
10 ** Geben Sie bei Eingabeaufforderung den Befehl ``elmalias'' ein\n
11 ** (keine ÿnderungen an der Alias-Textdatei erforderlich.)\n\nq
12 Yes\n\n** Alias-Datenbank wird neu erstellt...\n\n
13 \n** Problem bei Neuerstellung der Alias-Namen (%d ; bitte manuell beheben.)\n\n
14 \n** Alias-Namen neu erstellt, bitte Elm erneut aufrufen.\n\n
$set 5
1 Syntax: %s [-akKmz] [-f <Datei>] \n
2        %s [-s <Betreff>] <Namen>\n
3        %s -h\n\n
4 Warnung: System ohne Fehlerbehebung erstellt - Anforderung ignoriert\n
5 \n\rBetreff angegeben, aber keinen EmpfÃnger!\n\n\r
6 \n\rEmpfÃnger nicht angegeben oder unklar!\n\n\r
7 \nMŒgliche Startparameter fœr Elm-Programm:\n\n
8 \tPar.\t\t\tBedeutung\n
9 \t -a \t\tArrow - Pfeilzeiger trotzdem verwenden\n
10 \t -fx\t\tFile - Datei 'x' statt Mailbox lesen\n
11 \t -h \t\tHelp - Liste der Optionen anzeigen\n
12 \t -k \t\tSoftkeys off - Funktionstasten des Terminals HP 2622 ausschalten\n
13 \t -K \t\tKeypad&softkeys off - Funktionstasten und Ziffernblock abschalten\n
14 \t -m \t\tMenu - Menœ ausblenden, um œber mehr BildschirmflÃche zu verfœgen\n
15 \t -sx\t\tSubject 'x' - Betreff fœr Versand im Batch-Modus\n
16 \t -z \t\tZero - Elm nicht aufrufen, wenn keine Post vorliegt\n
$set 6
1  Noch %d Zeile(n) : weiter mit <Leertaste>, zurœck mit 'q'
2  %d Zeile(n) œbrig : weiter mit <Leertaste>, zurœck mit 'q'
3  %d Zeile(n) œbrig 
4 ...Suche nach Muster "%s"...
5 Muster nicht gefunden. Weiter mit beliebiger Taste
$set 8
1 Warnung: Systemverbindungen konnten nicht ermittelt werden...
$set 9
1 LŒschen des Alias-Namens:
2 Keine Alias-Namen fœr diesen Namen: %s
3 System-Alias kann nicht gelŒscht werden: %s
4 Alias-Textdatei kann nicht geŒffnet werden: %s
5 Alias-Textdatei konnte nicht geÃndert werden!
6 Link fehlgeschlagen! %s - %s
7 Mehrere Alias-Namen kŒnnen nicht gleichzeitig gelŒscht werden
$set 10
1 Warnung: Domain %s verwendet inaktives Feld!!
2 Kein Pfad zu %s gefunden!
3 Kein Pfad zu %s gefunden!
$set 11
1 Editor wird aufgerufen...
2 Warnung: Dateistatus nicht ermittelbar, Berechtigungen evtl. verfÃlscht
3 Probleme beim Aufruf des Editors %s!
4 Warnung: Neue Post ist angekommen...
5 TemporÃre Datei konnte nicht erneut geŒffnet werden.  Eingaben GEL⁄SCHT!
6 %s konnte zum Lesen nicht geŒffnet werden!  Eingaben GEL⁄SCHT!
7 Suche nicht bis Ende der Mailbox durchfœhrbar.  Eingaben GEL⁄SCHT!
8 TemporÃre Datei konnte nicht in Mailbox kopiert werden!
9 \n\r Kein Link zwischen %s und Posteingangsdatei %s herstellbar...\n\r
10 \n\r Kein interner Link zwischen %s und Posteingangsdatei %s herstellbar...\n\r
11 \n\rFœr Ihre Post mœssen Sie %s durchsehen...\n\r
12 ÿnderungen in die neue Post eingearbeitet...
13 Erneute Synchronisation mit neuer Version der Mailbox...
$set 12
1 Editor wird aufgerufen...
2 \r\nEditor wird aufgerufen...\r\n
3 Editor kann nicht aufgerufen werden - zu viele Optionen
4 Editor kann nicht aufgerufen werden - zu viele Optionen\r\n
5 Editor '%s' kann nicht aufgerufen werden zum Erstellen.
6 Editor '%s' kann nicht aufgerufen werden zum Erstellen.\r\n
7 %s konnte zum Anfœgen nicht geŒffnet werden
8 \n\nNachricht fortsetzen, '.' fœr Ende, %cp fœr Liste, %c? fœr Hilfe;\n\n
9 \n\nNachricht eingeben, '.' fœr Ende oder %c? <Return> fœr Hilfe;\n\n
10 (Fortsetzen)\n\r
11 (Eingabe der Nachricht fortsetzen, '.' fœr Ende)\r\n
12 \r\n('%s' im System nicht auffindbar!  Fortsetzen)\r\n
13 \r\nName des Editors eingeben : 
16 (Hier muﬁ ein bestimmter Befehl verwendet werden.  Fortsetzen...)\r\n
18 \r\n(Bedeutung von %c%c unbekannt. Mit %c? Hilfe aufrufen)\r\n
19 (Warnung: keine Anfœgung an Edierpuffer mŒglich - %s .....Fortsetzen.)
20 \r\n<Ende der Nachricht>\n\r\n\r
21 \r\nHier sind folgende Optionen verfœgbar:\n\n
22 \t%c?\tDieses Hilfemenœ anzeigen\n
23 \t~~\tZeile mit PrÃfixzeichen '~' hinzufœgen\n
24 \t%cb\tAdressen in der BCC-Verteilerliste Ãndern\n
25 \t%cc\tAdressen in der CC-Verteilerliste Ãndern\n
26 \t%ce\tFalls mŒglich, $EDITOR fœr Nachricht aufrufen\n
27 \t%cf\tListe mit Nachrichten oder aktuelle Nachricht hinzufœgen\n
28 \t%ch\tKopfdaten Ãndern: An,Kopie,Bkopie,Betreff (to,cc%s,subject)\n
29 \t%cm\tEntspricht '%cf', aber mit aktuellem PrÃfix\n
30 \t%co\tBenutzerdefinierten Editor fœr Nachricht aufrufen\n
31 \t%cp\tBisher eingegebene Nachricht anzeigen\n
32 \t%cr\tAngegebene Datei einlesen\n
33 \t%cs\tBetreff der Nachricht Ãndern\n
34 \t%ct\tAdressen in der An-Liste Ãndern\n
35 \t%cv\tEditor $VISUAL fœr Nachricht aufrufen\n\n
36 \t%c!\tUNIX-Befehl ausfœhren (oder Shell eingeben, falls kein Befehl)\n
37 \t%c<\tUNIX-Befehl ausfœhren und dessen Ausgabe an Nachricht anfœgen\n\n
38 \t.  \tin separater Zeile (oder Control-D) beendet die Nachricht\n\n
39 \r\n(Kein Dateiname fœr Einlesevorgang angegeben! Fortsetzen...)\r\n
40 \r\n(Datei '%s' konnte nicht gelesen werden! Fortsetzen...)\r\n
41 \r\n(Hinzufœgen nicht vollstÃndig mŒglich: %s)
42 \r\n(%d Zeile(n) aus Datei %s hinzugefœgt. Bitte fortfahren...)\r\n
43 \r\n(%d Zeile(n) an Nachricht angefœgt.  Bitte fortfahren...)\r\n
44 \r\nPanik: Kein Lesezugriff auf Datei! \r\n
45 \r\nPanik: Datei kann zum Anfœgen nicht wieder geŒffnet werden!\r\n
46 \r\n(Eingabe der Nachricht bitte fortsetzen)\r\n
47 (Einzubeziehende Nachricht(en) bitte angeben)\r\n
48 (Leider kein Zugriff auf Befehl 'readmail'...)\r\n
49 (Angeforderte Nachricht konnte nicht hinzugefœgt werden. Fortsetzen)\r\n
50 (Angeforderte Nachricht nicht vollstÃndig hinzugefœgt. Fortsetzen)\r\n
51 (%d Zeile(n) wurde(n) der Nachricht hinzugefœgt... Bitte fortfahren)\r\n
52 (Unterbrechung. Noch einmal, um Brief zu lŒschen...)\n\r
$set 13
1 Neue Post ist eingetroffen!
2 %d neue Nachricht(en) empfangen
3 Befehl: 
4 Befehl: 
5 Muster nicht gefunden!
6 Nachricht wird nach KalendereintrÃgen abgesucht...
7 Kalenderfunktionen leider inaktiviert
8 Nachricht weiterleiten, als ob nicht gelesen
9 Keine Post zum Weiterleiten!
10 Andere Mailbox
11 Keine Post zum LŒschen!
12 Mailbox bearbeiten
13 Mailbox ist leer!
15 Nachr. weiterleiten
16 Keine Post zum Weiterleiten!
17 Gruppenantwort
18 Bei einem Formular ist keine Gruppenantwort mŒglich!!
19 Adressen werden generiert...
20 Keine Post zum Beantworten!
21 Nachricht mit Kopfdaten...
22 Nachricht lesen
23 Angezeigte Nachrichten einschrÃnken durch...
24 Versenden
26 NÃchste Nachricht
27 Optionen
28 Post drucken
29 Keine Post zum Drucken!
30 Beenden
31 Neue Post!  Beenden abgebrochen...
32 Nachricht beantworten
33 Beenden abgebrochen...
34 Keine Post zum Speichern!
35 Nachricht speichern
36 (Liste der Mappen mit '?')
37 Keine Post zum Markieren!
38 Keine Post zum Markieren als nicht gelŒscht!
39 Keine Post zum Aufheben eines LŒschvermerks!
40 Ende
41 Keine Post zum Prœfen
42 Neue aktuelle Nachricht
43 Unbekannter Befehl: '?' fœr Befehlsœbersicht eingeben
44 Bereits bei erster Nachricht angelangt!
45 Keine Nachricht zum Lesen!
46 Nur %d Nachricht(en) gewÃhlt!
47 Nur %d Nachricht(en)!
48 Aktuelle Nachrichtennummer = %d\t\tinsgesamt %d Nachricht(en)\n
49 Kopfseite = %d           \t\tinsgesamt %d Seite(n)\n
50 \nAktuelle Posteingangsdatei ist %s.\n\n
51 Beliebige Taste drœcken, um fortzufahren: 
52 Keine Post zum Umleiten!
53 Mailbox geÃndert! --- Synchronisierung lÃuft
54 [%d message(s) %s %s, and %d deleted]
55 [Nachricht(en) in Ihrer mbox gespeichert]
56 [alle Nachrichten gelŒscht]
$set 14
1 Unbekannter Fehler (%d)
2 Unbekannter Fehler (%d) - keine Beschreibung
$set 15
1 Nachricht(en) ablegen in: 
3 Konnte nicht an '%s' anfœgen: %s
4 Konnte nicht vollstÃndig an '%s' anfœgen: %s
5 Nachricht(en) gespeichert
6 Nachricht %d angefœgt an Datei '%s'
7 Nachricht %d gespeichert in Datei '%s'
8 MAILDIR nicht definiert. '%c' kann nicht erweitert werden.
9 Wert von $%s ist unbekannt!
10 Ungœltiger Dateiname.  Nachricht wird nicht gespeichert.
11 'popen'-Fehler. Nachricht wird nicht gespeichert.
$set 16
1 \n\rFehler bei 'fstat' auf Datei %s!\n\r
3 Datei %s konnte nicht geŒffnet werden.
4 Datei %s existiert bereits!  Wird œberschrieben...\n
5 Keine Post zu lesen.\n
$set 17
1 ELM [Suchen] - Fehler beim Versuch, %d Bytes in Datei einzulesen
$set 18
1 Fehler %s beim Versuch, %s zur Feldprœfung zu Œffnen!
2 Nachricht kann zur Formularprœfung nicht gelesen werden!
3 ⁄ffnen einer neuen Formulardatei zur Formularausgabe nicht mŒglich!
4 Erstellen einer temporÃren Formulardatei %s fehlgeschlagen: %s
5 Erstellen einer temporÃren Formulardatei %s fehlgeschlagen: %s
7 Fehler %s beim Aufheben des Links von Datei %s
8 Fehler %s beim Erstellen des Links von %s auf %s
10 ELM [Suchen] - Fehler beim Einlesen von %d Bytes in Datei (%s)
11 Kein Formular in dieser Nachricht!?
13 Falsch strukturiertes Formular. Keine Antwort mŒglich!
15 "%s" als Ausgabedatei kann nicht geŒffnet werden! (%s)
16 (Anfœgen an Edierpuffer fehlgeschlagen: %s)\r\n
17 (Zeilen nacheinander eingeben und mit '.' auf separater Zeile abschieﬁen)\n
18 (Anfœgen an Edierpuffer fehlgeschlagen: %s)\r\n
19 Anfœgen an Edierpuffer fehlgeschlagen: %s
$set 19
1 Auswahl: 
2  (Prœfung lÃuft) 
5 Angegebener Kopf ist unbekannt!
6 Bildschirm zum Bearbeiten des Nachrichtenkopfs
7 Kennbuchstaben der Kategorie wÃhlen, U=Benutzerdefinierter Kopf oder <Return>
9 In wieviel Tagen soll diese Nachricht verfallen?
10 Unlogisch!
11 Ablauftermin muﬁ innerhalb von acht Wochen nach heutigem Datum liegen
12 Kopf kann nicht mit Leerzeichen beginnen!
13 Doppelpunkt darf nicht als erstes Zeichen verwendet werden!
14 Feldname muﬁ mit Doppelpunkt beendet werden!
15 Kopfname darf keine Leerstellen enthalten!
16 Zu viele Namen. Namen unter 'Kopie' ignoriert !
17 S Betreff: 
18 B Bkopie: 
19 C Kopie: 
20 E Ablauf: %s
21 A Aktion: 
22 P PrioritÃt: 
23 R Als Antwort auf: %s
24 T An: %s
25 C Kopie : %s
26 B Bkopie: %s
27 R Antwort auf: %s
28 S Betreff: %s
29 A Aktion: %s
30 E Ablauf: %s
31 P PrioritÃt: %s
32 Aktuelle Nachricht Nr. :
$set 20
1 \r\nElm-Kopfdaten-Editor. Bitte aus folgenden Optionen wÃhlen:\n\r\n
2 Auswahl? q%c
3 \r\nZu bearbeitende Kopfdaten ('?' zeigt Auswahl an) : q%c
4 Kopfdaten anzeigen:\r\n\n
5 Kopfdaten-Editor verlassen\r\n
6 Hilfe\r\n\n
7 %c\r\nAuswahl nicht verstanden!\r\n
8 In wieviel Tagen soll diese Nachricht ablaufen?
9 Abgelaufene Nachrichten kŒnnen nicht verschickt werden!\r\n
10 Ablauftermin muﬁ innerhalb von acht Wochen nach heutigem Datum liegen\r\n
11 \rZu viele Namen. Namen unter 'Kopie' ignoriert !\r\n
12 \rZu viele Namen. Namen unter 'Bkopie' ignoriert !\r\n
$set 21
1 ELM-Hilfesystem
2 Taste, fœr die Hilfe gewœnscht wird, '?' fœr Liste oder '.' zum Verlassen 
3 Hilfe fœr Taste: 
5 Unbekannter Befehl. Mit '?' kann Befehlsliste angezeigt werden...
6 $ = Zwangssynchronisierung der aktuellen Mailbox. GelŒschte Post geht verloren
7 ! = Zurœck zu beliebiger UNIX-Shell oder einfach Befehl eingeben
9 | = Aktuelle oder markierte Nachricht(en) umleiten mit angegebenem Befehl
12 * = Zur letzten Nachricht in aktueller Mailbox
13 - = Zur vorigen Nachrichtenseite in aktueller Mailbox
14 '=' = Zur ersten Nachricht in aktueller Mailbox
15 + = Zur nÃchsten Nachrichtenseite in aktueller Mailbox
16 / = Suche nach angegebenem Muster in Mailbox
17 < = Aktuelle Nachricht nach KalendereintrÃgen (sofern aktiviert) absuchen
20 a = Zum Alias-Untermenœ. Alias-Namen erstellen und anzeigen
21 b = Weiterleitung einer Nachricht an Dritte, als ob nicht gelesen
22 c = Andere Mailbox aufrufen. Aktuelle Mailbox wie mit "Quit" verlassen
23 d = Aktuelle Nachricht fœr spÃteres LŒschen markieren
24 ^D = Alle Nachrichten mit angegebenem Muster zum LŒschen markieren
26 f = Aktuelle Nachricht weiterleiten, Rœcksendeadresse ist Ihre Adresse
27 g = Gruppenantwort nicht nur an Sender, sondern an alle EmpfÃnger
28 h = Nachricht mit allen Kopfdaten anzeigen (Weedout-Liste ignorieren)
29 j = Zur nÃchsten Nachricht.  Entspricht Pfeil nach UNTEN
30 k = Zur vorigen Nachricht.  Entspricht Pfeil nach OBEN
31 l = EinschrÃnkung der Nachrichtenanzeige nach angegebenen Kriterien
32 m = Post erstellen und an angegebene(n) Person(en) senden
33 n = NÃchste Nachricht lesen, dann nÃchste Nachricht auf aktuell setzen
34 o = Zum Untermenœ "Optionen"
35 p = Aktuelle Nachricht oder markierte Nachrichten drucken
36 q = Mail-System verlassen, LŒschungen, Speicherungen usw. berœcksichtigen
37 r = Antwort auf Nachricht. Geht nur an Sender der Nachricht
38 s = Aktuelle oder markierte Nachricht(en) in angegebener Datei speichern
39 t = Nachricht fœr weitere VorgÃnge markieren (oder Markierung entfernen)
40 ^T = Alle Nachrichten mit angegebenen Muster markieren
41 u = Wiederherstellen - LŒschmarkierung der Nachricht entfernen
42 x = Mail-System ohne weitere Bearbeitung verlassen
43 <Return> = Aktuelle Nachricht lesen
44 ^L = Bildschirmanzeige neu aufbauen
45 Mail-System ohne weitere Bearbeitung verlassen
46 <Nummer> = Nachricht mit angegebener Nummer wird aktuelle Nachricht
47 Hilfedatei %s konnte nicht geŒffnet werden
48 Weiter mit beliebiger Taste: 
49 Zurœck mit beliebiger Taste: 
50 ^U=LŒschen rœckgÃngigmachen ('D' von allen Nachr. mit angeg. Muster entfernen)
$set 22
1 Datei %s konnte nicht fœr Fehlerbehebungsausgabe geŒffnet werden!\n
2 Um das Programm "elm" benutzen zu kŒnnen, mœssen Sie Ihren Terminaltyp\n
3 angeben...(die Umgebungsvariable muﬁ auf "TERM" gesetzt sein)\n
4 Fœr "elm" wird ein cursor-adressierbares Terminal benŒtigt.\n
5 Keinen "termcap"-Eintrag fœr "%s" gefunden - "TERM"-Einstellung prœfen!...\n
6 Terminal-Eintrag konnte nicht initialisiert werden: Rœckgabecode %d unbekannt\n
7 Kennwort-Eingabe nicht lesbar??
8 Mailbox '%s' kann nicht geŒffnet werden:%s\n
9 \n\r\n\rUrsprœngliche Kopfdaten konnten nicht zugeordnet werden!\n\r\n
10 \n\r\n\rKopf-Index konnte nicht angezeigt werden!\n\r
11 \n\rMehr Spalten im Fenster erforderlich!\n\r
$set 23
1 Nachricht(en) lŒschen? (y/n) 
2 Nichts gelŒscht
3 Alle Nachrichten lŒschen? (y/n) 
4 Nichts gelŒscht
5 Post in Eingangs-Mailbox ablegen? (y/n)
6 Mailbox unverÃndert
7 Neue Post soeben eingetroffen - Synchronisierung lÃuft...
8 [%d Nachricht(en) beibehalten, %d gelŒscht]
9 [alle Nachrichten werden beibehalten]
15 [alle Nachrichten werden gelŒscht]
16 [keine Nachrichten an %s, keine zu lŒschenden Nachrichten]
17 beibehalten
18 gespeichert
19 Anfœgen an %s nicht mŒglich: %s\n
20 Mailbox %s unverÃndert: %s\n
21 Fehler %s (%s) bei stat(%s)
23 Postdatei konnte nicht geÃndert werden!
24 Schwerwiegendes Problem!!!
25 Post in %s gespeichert
26 Link fehlgeschlagen! %s - %s
27 ..leere Postdatei '%s' wird beibehalten..
29 \n\rFehler beim Versuch, die Sperrdatei %s zu erstellen;\n\r
30 Sperrdatei kann nicht erstellt werden! Erfordert Schreibberechtigung in "%s"\n\r
31 Post wird empfangen...\t\twarten...1 
32   Versuch Nr.%d: Post wird empfangen...warten
33 \nZeitgrenze œberschritten - aktuelle Sperrdatei wird gelŒscht...
34 Aktuelle Sperrdatei wird gelŒscht!
35 \n\rAktuelle Sperrdatei %s konnte nicht gelŒscht werden\n\r
36 \n\r\n\rNach %d Wiederholungen aufgegeben...\n\r
37 \n\rBitte versuchen Sie in einigen Minuten, Ihre Post wieder zu lesen.\n\r\n\r
38 Zeitgrenze fœr LesevorgÃnge auf Sperrdatei œberschritten. Programm beendet.
$set 24
1 Auswahlkriterien liegen bereits vor - weitere gewœnscht? (y/n) n%c
2 Kriterien werden hinzugefœgt...
4 Kriterien eingeben: 
5 "%s" wird als Auswahlkriterium nicht verstanden!
6 keine Elemente gewÃhlt
7 %d Element(e) gewÃhlt
$set 25
1 Post nicht verschickt
2 \r\n\r\nPostvorgang abgebrochen!\n\r
3 \n\rKein Betreff - soll mit Nachricht fortgefahren werden? (y/n) n%c
4 Kein Betreff - soll mit Nachricht fortgefahren werden? (y/n) n%c
5 \n\r\n\rPost nicht verschickt!\n\r
6 Zu viele Namen. Namen unter 'Kopie' ignoriert
7 Zu viele Namen. Namen unter 'Bkopie' ignoriert
8 Nachricht kopieren? (y/n) 
9 Nachricht senden an: 
10 An: 
11 Betreff der Nachricht: 
12 Betreff: 
13 Betreff: <keiner>
14 Kopien an: 
15 Blindkopien an: 
16 An: (%s)
17 An: %s
18           An: %s
$set 26
1 Datei %s konnte nicht erstellt werden (%s)\r\n
2 Datei %s konnte nicht erstellt werden (%s)
3 %s konnte leider nicht œbertragen werden\n
4 Formulardatei nicht gefunden!
5 Warnung - Unterschriftsdatei %s konnte nicht angefœgt werden\r\n
6 Warnung - Unterschriftsdatei %s konnte nicht angefœgt werden
7 Warnung - Unterschriftsdatei %s konnte nicht komplett angefœgt werden\r\n
8 Warnung - Unterschriftsdatei %s konnte nicht komplett angefœgt werden
13 Post ohne "An:"-Kopf nicht verschickt! 
14 Antwortdatei (%s) konnte nicht geŒffnet werden\r\n
15 Antwortdatei (%s) konnte nicht geŒffnet werden
16 Antwortdatei (%s) konnte nicht geŒffnet werden\n
17 Zu versendende Nachricht konnte nicht in %s kopiert werden : %s\r\n
18 Zu versendende Nachricht konnte nicht in %s kopiert werden : %s
19 Nachricht wurde mit anderem Postprogramm verschickt - %s\r\n
20 Nachricht wurde mit anderem Postprogramm verschickt - %s
21 Post wird verschickt ...
23 \rPost verschickt!!      \n\r
24 Post verschickt!
25 Soll stattdessen letzte Nachricht aufgerufen werden? (y/n) y%c
26 Soll letzte Nachricht aufgerufen werden? (y/n) y%c
27 \n\rFolgende Optionen stehen zur Wahl:\n\r
28 S=Versenden, E=Edieren, H=Kopfdaten Ãndern/hinzufœgen oder F=LŒschen\n\r
29 \n\rWas ist Ihre Wahl? s%c
30 Nachricht leider nicht gespeichert.\n\r\n\r
32 Nachricht in Datei "$HOME/%s gespeichert"\n\r\n\r
33 \n\rNicht verstanden, bitte Antwort erneut eingeben...\n\r
34 WÃhlen Sie eine der folgenden Optionen durch Eingabe des ersten Buchstabens:
35 E=Edieren, H=Kopfdaten edieren, S=Versenden oder F=LŒschen: s%c
36 Optionen: H=Kopfdaten, S=Versenden oder F=LŒschen: s%c
37 Optionen: E=Edieren, H=Kopfdaten, S=Versenden, F=LŒschen : s%c
38 Optionen: E=Edieren, H=Kopfdaten, M=Formular, S=Versenden, F=LŒschen : s%c
39 Bitte wÃhlen: E=Edieren, H=Kopfdaten, S=Versenden, F=LŒschen : s%c
40 Nachr. beibeh. - wiederherst. beim nÃchsten F(Weiter), M(Versenden), R(Beantw.) 
41 Keine Felder im Formular!
42 Fehler %s beim Versuch, in %s zu schreiben\r\n
43 Fehler %s beim Versuch, in %s zu schreiben
44 Soll eine Kopie vom entfernten Rechner zurœckkommen? (y/n) 
45 Yes (Ja) - Kopie soll zurœckkommen\n\r
46 Soll eine Kopie vom entfernten Rechner zurœckkommen? (y/n) n%c
$set 27
1 Warnung: Leerzeile in %s ignoriert!\r\n
2 Warnung: Leerzeile in %s ignoriert!
3 Warnung: Befehl in `Anfœhrungszeichen` im Kopf konnte nicht ausgefœhrt werden\r\n
4 Warnung: Befehl in `Anfœhrungszeichen` im Kopf konnte nicht ausgefœhrt werden
5 Befehl "%s" in `Anfœhrungszeichen` in .elmheaders fehlgeschlagen\r\n
6 Befehl "%s" in `Anfœhrungszeichen` in .elmheaders fehlgeschlagen
$set 28
1      Post wird bereits von einem anderen Elm-Benutzer gelesen!\n\r
2 \n\r     [Falls dies ein Fehler ist, muﬁ '%s/%s%s' gelŒscht werden]\n\r
3 Mailbox '%s' kann nicht geŒffnet werden (%s)
4 (Liste der Mappen mit '?')
5 Name der neuen Mailbox:
6 \n\rName der neuen Mailbox:
8 Diese Mailbox wird bereits gelesen!
9 Datei '%s' kann nicht geŒffnet werden: %s
10 Datei wird neu synchronisiert
11 Mailbox: %s
12 Link auf temporÃre Postdatei [%s] kann nicht entfernt werden\n\r
13 %s wird eingelesen, Nachricht: %d
14 %s wird eingelesen, Nachricht: 0
15 Lese Nachricht: 0
17 Die temporÃre Mailbox existiert bereits !!
18 Beendigung ohne Eingriff...
19 \nDatei %s konnte nicht als temporÃre Mailbox geŒffnet werden;\n
20 \nDatei %s konnte nicht als temporÃre Mailbox erneut geŒffnet werden;\n
21 \nSuche bis %ld (Ende der Mailbox) in %s nicht durchfœhrbar!\n
22 Post wird an %s weitergeleitet
23 \n\rPostdatei ist defekt!! Kann nicht gelesen werden!!\n\r\n\r
24 Datei %s konnte nicht in temporÃre Mailbox %s kopiert werden
25 Beendigung ohne ÿnderung an der Mailbox...
26 \n\r\n\rNicht genœgend SpeicherkapazitÃt verfœgbar! Fehler bei Nachricht #%d\n\r\n\r
27 \n%s konnte nicht als temporÃre Postdatei geŒffnet werden;\n
28 Postdatei ist defekt!! Nicht lesbar!!
29 Ungœltiger Dateiname. Mailbox kann nicht geÃndert werden.
30 Fehler bei 'popen'. Mailbox kann nicht geÃndert werden.
$set 29
1 Kennbuchstaben der Optionszeile angeben, '>' fœr Speichern oder R fœr Return
2 Befehl:
3 Optionen werden in .elm/elmrc gespeichert...
4 Unbekannter Befehl!
5 -- Elm Optionen-Editor --
6 ÿndern mit Leertaste, zurœck mit beliebiger Taste
7 ÿndern mit Leertaste
8 Leertaste=vorwÃrts  R=rœckwÃrts
9 Mailbox wird neu sortiert...
10 Diese Sortierung ordnet aufsteigend nach Versandalter (neu bis alt)
11 Diese Sortierung ordnet aufsteigend nach Empfangsalter (neu bis alt)
12 Diese Sortierung ordnet aufsteigend nach Hinzufœgung an Mailbox (neu bis alt)
13 Diese Sortierung ordnet nach Absender (umgekehrte alphabetische Reihenfolge)
14 Diese Sortierung ordnet Nachrichten nach LÃnge (lang bis kurz)
15 Diese Sortierung ordnet nach Betreff (umgekehrte alphabetische Reihenfolge)
16 Diese Sortierung ordnet umgekehrt nach Status - gelŒscht bis markiert...
17 Diese Sortierung ordnet absteigend nach Versandalter (alt bis neu)
18 Diese Sortierung ordnet absteigend nach Empfangsalter (alt bis neu)
19 Diese Sortierung ordnet absteigend nach Hinzufœgung an Mailbox (alt bis neu)
20 Diese Sortierung ordnet nach Absender
21 Diese Sortierung ordnet Nachrichten nach LÃnge (kurz bis lang)
22 Diese Sortierung ordnet Nachrichten nach Betreff
23 Diese Sortierung ordnet nach Status - markiert bis gelŒscht...
24 In dieser Datei werden KalendereintrÃge der Nachrichten gespeichert.
25 Programm zur Anzeige einzelner Nachrichten (geben Sie z.B. 'builtin' ein)
26 Editor fœr das Versenden von Nachrichten usw.
27 Dieses Ablageverzeichnis wird verwendet, wenn '=' (etc) in Dateinamen steht
28 Bestimmt, ob das Mini-Menœ angezeigt wird oder nicht
29 Bestimmt, ob Namen und Adressen der Post oder nur Namen angezeigt werden
30 Hier werden Kopien der zu versendenden Nachrichten automatisch gespeichert.
31 So werden Ausdrucke erzeugt.  "%s" wird durch den Dateinamen ersetzt.
32 Hierdurch werden die Sortierkriterien fœr die Mailboxes angegeben.
33 So wird Ihr voller Name beim Postversand registriert.
34 Bestimmt, ob der ELM-Cursor ein Pfeil oder ein Unterlegungsbalken ist.
35 Ihr Kenntnisstand œber das elektronische Postsystem ELM.
36 Taste, fœr die Hilfe gewœnscht wird, '?' fœr Liste oder '.' zum Verlassen 
37 Taste : 
38 %c wird in diesem Abschnitt nicht verwendet.
39 %s = Zurœck zum Hauptmenœ mit den Optionswerten
40 KEINE Eingabe, Standardwert wird verwendet!
41 Ablageverzeichnis nicht gefunden -- %s
42 Soll Verzeichnis erstellt werden (y/n)? y%c
43 Verzeichnis konnte nicht erstellt werden, Standardwert wird verwendet!
44 Standardwert wird verwendet!
45 Vorherigen Wert verwenden
46 Bestimmt, ob beim Verschicken von Post Tabs in Leerzeichen umgewandelt werden
47 C Kalenderdatei      : %s
48 D Post anzeigen mit  : %s
49 D Post anzeigen mit  : %s
50 E Editor             : 
51 E Editor             : %s
52 F Mappenverzeichnis  : 
53 F Mappenverzeichnis  : %s
54 S Sortierkriterium   : %s
55 O Postausgang speich.: 
56 O Postausgang speich.: %s
57 P Post drucken mit   : 
58 P Post drucken mit   : %s
59 Y Ihr voller Name    : 
60 Y Ihr voller Name    : %s
61 A Cursor als Pfeil   : %s
62 M Menœanzeige        : %s
63 U Benutzerebene      : %s
64 N Nur Namen          : %s
65 T Tabs zu Leerzeichen: %s
$set 30
1 Befehl: 
2 %s Nachrichten entsprechend dem Suchmuster...
3 Einige Nachrichten sind bereits markiert
4 Eine Nachricht ist bereits markiert
5 - Markierung(en) lŒschen? y%c
6 Muster eingeben: 
7 %s %d Nachricht(en)
8 Markiert 
9 Zum LŒschen markiert: 
10 LŒschen rœckgÃngig gemacht
11 Keine €bereinstimmung. %s keine Nachrichten
12 / = €bereinstimmung in allen Nachrichten
13 Suchmuster: 
14 Suchmuster (in gesamter Mailbox):
15 tritt nur in der aktuellen Nachricht auf
16 Mailbox wird nach Muster durchsucht...
17 ELM [Suchmuster]: Fehler beim Lesen von %ld Bytes in Datei (%s)
$set 31
1 \n\r\n\rSpeicherzuweisung ("malloc") bei %d Bytes nicht durchfœhrbar!!\n\r\n\r
$set 32
1 \n\rWarnung:\n\r
2 Alle mit Punkt beginnenden Elm-Dateien werden inzwischen in ihrem eigenen\n\r
3 Verzeichnis - $HOME/.elm - erwartet. Sollen sie dorthin gestellt werden?\n\r
4 [Falls nicht, werden die Dateien ignoriert] (y/n)? y%c
5 \n\rNicht umgestellt.\n\r
6 Zeile %d Ihrer Datei ".elm/elmrc" hat falsche Syntax:\n> %s\n
7 Fehler beim Lesen der Datei '.elm/elmrc';\n
8 Zeile %d: Sortierschlœssel '%s' ist unbekannt!\n
9 "Unterschrift" in Datei .elm/elmrc ist veraltet - ignoriert!\n\r
10 \t(Unterschrift sollte zu verwendenden Dateinamen statt ein/aus angeben\n\r\n
11 Warnung: Anzahl der Zwischenstopps liegt œber %d - Ignoriert.\n
12 Warnung: Zeitgrenze liegt unter 10 Sekunden - Ignoriert.\n
13 Zeile %d in Datei ".elm/elmrc" ist unklar:\n> %s\n
14 Zu viele Weed-KŒpfe! Abbruch...\n
15 Zu viele Weed-KŒpfe! - kein freier Speicherplatz! Abbruch...\n
16 \n\rZu wenig freier Speicherplatz fœr Weed-Liste. Abbruch\n\r
17 \n\rUmgebungsvariable '%s' kann nicht erweitert werden\n\r
18 \n\rUmgebungsvariable '%s' nicht erweiterbar - Dateiname zu lang
19 Elm benŒtigt Verzeichnis $HOME/.elm zum Speichern der elmrc- und Alias-Dateien\n\r
20 Soll das Verzeichnis jetzt angelegt werden (y/n)? y%c
21 \n\rAkzeptiert.\tWird ausgefœhrt.\n\r
22 \n\rVerzeichnis konnte nicht erstellt werden.   * %s *\n\r
23 \n\rAkzeptiert.\tSpÃter kŒnnen allerdings Probleme auftreten.\n\r
24 Ablageverzeichnis nicht gefunden -- %s\n\r
$set 33
1 Datei %s fœr Schreibvorgang konnte nicht geŒffnet werden (%s)
2 Anfœgung an Datei %s nicht durchfœhrbar: %s
3 Soll diese Nachricht wirklich nochmals verschickt werden (y/n) ? y%c
4 Weiterleiten der Nachricht abgebrochen
5 Post wird erneut verschickt...
6 Post erneut verschickt
7 Zu viele Adressen zum Weiterleiten - Post nicht verschickt !
$set 34
1 <Bearbeiten nicht erlaubt>
2 Zu versendende Nachricht bearbeiten (y/n) ? 
3 ELM [Suchen]: Fehler beim Einlesen von %d Byte in Datei (%s) 
4 Rœcksprungadresse kann nicht erstellt werden - MAX_HOPS-Limit erreicht!
$set 35
1 %d Bytes konnten nicht in Datei (%s) gelesen werden
2 %d Bytes konnten nicht in Datei (%s) gelesen werden
$set 36
1 Warnung: Speicherung ohne Kommentare - kein Zugriff auf %s
2 Konfiguration kann nicht gespeichert werden: kein Schreibzugriff auf %s [%s]
3 Optionen in Datei %s gespeichert
4 "%s" nicht gespeichert..... Fortsetzen
5 "Weedout-Liste" konnte nicht vollstÃndig gespeichert werden
6 "Alternativen" konnten nicht vollstÃndig gespeichert werden
7 Konfiguration kann nicht gespeichert werden: Pfadname zu lang
$set 37
1 Variable 'SAVEFILE' nicht definiert!\r\n
2 Variable 'SAVEFILE' nicht definiert!
3 Keine Anfœgung an %s : %s\r\n
4 Keine Anfœgung an %s : %s
5 Datei %s konnte nicht gelesen werden: %s\r\n
6 Datei %s konnte nicht gelesen werden: %s
$set 38
1 Mailbox ist '%s' [Elm %s]
3 Mailbox ist '%s', wobei %d von %d gezeigt werden [Elm %s]
4 Mailbox ist '%s' mit %d Nachricht(en) [Elm %s]
5 WÃhlen Sie einen der folgenden Befehle durch Eingabe des angegeb. Buchstabens:
6 D=LŒschen, U=LŒschen aufheben, M=Versenden, R=Beantw., F=Weiterleiten, Q=Beenden
7 Lesen einer Nachricht mit <Return>.  j = nach unten, k = nach oben, ? = Hilfe
8 |=Pipe, !=Shell, ?=Hilfe, <n>=aktuelle Nachricht auf n setzen, /=Suchmuster
9 A=Alias,C=Andere Mailbox,D=LŒschen,F=Weiter,G=Grp.antw.,M=Versenden,N=NÃchste,
10 O=Opt.,P=Drucken,R=Beantw.,S=Speich.,T=Mark.,Q=Beenden,U=LŒ. aufheben, X=Ende
$set 39
1 Nur %d Nachrichten!
2 DIESE Nachricht ist nicht lesbar!
3 ELM hat beim Suchen %d Bytes nicht in Datei (%s) gelesen
4 Pipe zu %s [%s] kann nicht erstellt werden
5 %s(Nachricht adressiert an %.60s)\n
6 %s(Nachricht adressiert an %s)\n
7 \n(** Diese Nachricht ist markiert als persŒnlich
8 \n(** Diese Nachricht ist markiert als vertraulich
9 \n(** Diese Nachricht ist markiert als eilig
10 , eilig
11  und eilig
12 \n(** Diese Nachricht ist abgelaufen
13 , und sie ist abgelaufen
14 Synchronisationsfehler: Mailbox kann nicht nochmals geŒffnet werden!!
15  Drœcken Sie <Return>, um zum Hauptmenœ zurœckzukehren : 
$set 40
1 Befehl: Pipe
2 Befehl: Systemaufruf
3 Suche nach KalendereintrÃgen nicht durchfœhrbar!
4 Befehl: Nachricht weiterleiten, als ob nicht gelesen
5 Nachricht zum LŒschen markiert
6 Nachricht zum LŒschen markiert. Befehl ? 
7 Befehl: Nachr. weiterleiten
8 Befehl: Gruppenantwort
9 Befehl: Nachricht verschicken
11 In Druckwarteschlange. Befehl ? 
12 Befehl: Nachricht beantworten
13 Befehl: Nachricht speichern
14 Nachricht markiert
15 Nachricht markiert.  Befehl ? 
16 LŒschen der Nachricht aufgehoben
17 LŒschen der Nachricht aufgehoben.  Befehl ? 
18 Anforderung (I=zurœck zur Indexseite) ? 
$set 41
1  Nachr.  lesen
2  Nachr.  senden
3  Nachr. beantw.
4  Nachr. speich.
5  Nachr. lŒschen   
6 LŒschen aufheben
7  Andere Mailbox
9 LŒschen/L.aufheb
10  Nachr. drucken
11  Elm    beenden
12 Alias f.akt.Nchr
13  Namen   prœfen
14  System anzeigen
15  Alias  lŒschen
16  Alias  erstell.
17  Haupt-   menœ 
18   Ja
19   Nein
20 NÃchst.  Seite  
21 NÃchst.  Nachr. 
22 Vorher.  Nachr. 
25  Nachr.  senden 
27  Post-  Verzeich
28  Home-  Verzeich
29 EingangsMailbox 
30 Abbruch
32 LŒschen
33 Kopfdat. bearb. 
34  Senden
35 Formul.  bearb.
38  Nachr.  bearb.
41  Senden
42  Nachr.  bearb.
43 Kopfdat. bearb.
44  Senden
45 Erweit. Adresse
46 Benutzeranzeigen
47 Betreff
48   Von
49    An
50 Einschr.aufheben
$set 42
1 Nachrichten werden nach %s sortiert
$set 43
1 "get_token"-Aufrufe tiefer als %d verschachtelt!
2 MŒgliche Alias-Schleife. Alias nicht in %d Erweiterungen auflŒsbar!
$set 44
1 (Shell-Name fœr eine Shell verwenden)
2 Shell-Befehl: 
3 \n\n(Bildschirminhalt kann bei Bedarf mit ^L wiederhergestellt werden)\n
4 \n\nBeliebige Taste drœcken, um zu ELM zurœckzukehren: 
5 Rœckgabecode war %d
6 Markierte Nachricht umleiten nach: 
7 Markierte Nachricht umleiten nach: 
8 Aktuelle Nachricht umleiten nach : 
9 \n\nBeliebige Taste drœcken, um zu ELM zurœckzukehren: 
11 Druck nicht durchfœhrbar - Option "printmail" nicht definiert!
12 In Warteschlange...
13 Nachricht(en) wartet/warten auf Druck
14 Druck fehlgeschlagen, Rœckgabecode %d
15 \n\rInhalt des Ablageverzeichnisses:\n\r\n\r
16 Zu viele Nachrichten zu drucken - nur %d Nachr. in Warteschlange gestellt
17 Zu viele Nachrichten zu drucken oder Druckbefehl zu lang
18 Zu viele Nachrichten oder Pipe-Befehl zu lang
$set 45
1 \nNotabbruch!  Alle temporÃren Dateien intakt!\n\n
2 Fehler: Status-Keller defekt
$set 46
1 \n\nGestoppt.  Mit "fg" zurœck zu Elm\n\n
2 Wieder in Elm. (Bildschirmauffrischung evtl. explizit anfordern!)
3 \n\nSignal fœr ungœltigen Befehl!\n\n
4 \n\nSignal fœr Gleitkomma-Ausnahme!\n\n
5 \n\nSignal fœr Bus-Fehler!\n\n
6 \n\nSignal fœr Segmentœberschreitung!\n\n
