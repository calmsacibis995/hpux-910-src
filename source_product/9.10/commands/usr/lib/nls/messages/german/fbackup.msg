$set 1
1 fbackup(1001): Nicht gen?gend virtueller Speicher\n
2 fbackup(1002): Sicherung nicht abgeschlossen: Lese- oder Schreibproze? abgebrochen\n
3 fbackup(1003): Nicht gen?gend virtueller Speicher\n
4 fbackup(1004): Session beginnt um %s\n
5 fbackup(1005): Laufzeit: %d Sekunden\n
6 fbackup(1006): Zu viele Leser angegeben\n
7 fbackup(1007): Zu viele Speichers?tze angegeben\n
8 fbackup(1008): Auf aktuelles Arbeitsverzeichnis kann nicht zugegriffen werden\n
9 fbackup(1009): Auf angegebenes gemeinsam benutztes Speichersegment kann nicht zugegriffen werden\n
10 fbackup(1010): semget nicht erfolgreich f?r die Semaphore\n
11 fbackup(1011): shmat nicht erfolgreich f?r das gemeinsam benutzte Speichersegment\n
12 fbackup(1012): Session wieder erfolgreich gestartet\n
13 fbackup(1013): Es ist nicht m?glich, die Session wieder zu starten\n
14 fbackup(1014): Erstellung eines neuen Prozesses fehlgeschlagen f?r Leser %d\n
15 fbackup(1015): Erreicht nach Leseausf?hrung (reader execl)\n
16 fbackup(1016): semctl fehlgeschlagen\n
17 fbackup(1017): Erstellung eines neuen Prozesses fehlgeschlagen f?r Schreiber\n
18 fbackup(1018): Erreicht nach Schreibausf?hrung (writer execl)\n
19 fbackup(1019): Warnung: Keine der angegebenen Dateien mu?te gesichert werden\n
20 fbackup(1020): Unerwartetes Beenden\n
21 fbackup(1021): Dateiliste wiederhergestellt\n
22 fbackup(1022): Wiederherstellung der Dateiliste nicht m?glich\n
23 fbackup(1023): M?chten Sie diese Session sp?ter erneut starten?\n
24 fbackup(1024): Dieser Status der Session wurde gesichert\n
25 fbackup(1025): Status der Session nicht gesichert\n
26 fbackup(1026): Austritt ohne den Status der Session zu sichern\n
27 fbackup(1027): Sicherungslauf nicht abgeschlossen: Lese- oder Schreibproze? abgebrochen\n
28 fbackup(1028): SIGTERM erhalten; Bereinigung der Situation und Beenden\n
29 fbackup(1029): Unerwartetes Ende\n
101 fbackup(1101): Unerwarteterweise keine Dateien zum Sichern mehr\n
102 fbackup(1102): Filestatus von %s konnte nicht festgestellt werden\n
103 fbackup(1103): Datei %s kann nicht ge?ffnet werden\n
104 fbackup(1104): Nicht gen?gend virtueller Speicher\n
105 fbackup(1105): Verzeichnis %s kann nicht ge?ffnet werden\n
106 fbackup(1106): Nicht gen?gend virtueller Speicher\n
107 fbackup(1107): Keine Datei-Kontrollinformation f?r Datei %s\n
108 fbackup(1108): Datei %s ist gesperrt\n und wird nicht gesichert\n
201 fbackup(1201): Zugriffs- und Modifikationszeit der Datei %s k?nnen nicht zur?ckgesetzt werden\n
202 fbackup(1202): Es wurde unbekannte/r Benutzer oder Gruppe in ACL f?r\nDatei: %s angegeben; optionale Eintr?ge werden nicht gesichert.\n
301 fbackup(1301): Fortsetzung bei Datei %d\n
401 fbackup(1401): Zus?tzliches Argument: %s ignoriert\n
402 fbackup(1402): Bitte geben Sie nicht mehr als eine fbackup-Ebene an\n
403 fbackup(1403): Mindestens eine Ausgabedatei mu? angegeben werden\n
404 fbackup(1404): Keine Dateien spezifiziert; (verwenden Sie mindestens eine der Optionen -i und /oder -g)\n
405 fbackup(1405): Keine der Optionen [0-9ieg] d?rfen mit -R\n verwendet werden.
406 Syntax: /etc/fbackup -f ger?t [-f ger?t] ... [-0-9] [-nsuvyH]\n
407 \t[-i pfad] ... [-e pfad] ... [-g graph] ... [-d pfad] [-I pfad] [-V pfad] [ -c konfig]\n
408 \nSyntax: /etc/fbackup -f ger?t [-f ger?t] ... -R neustart [-nsuvyH]\n
409 \t[-d pfad] [-I pfad] [-V pfad] [-c konfig]\n\n
410 fbackup(1410): Damit die Aktualisierung (-u) durchgef?hrt wird, sollte genau eine "graph"-Datei (-g)\n
411 \tund keine Include- oder Exclude-Datei (-i, -e) angegeben werden.\n
412 fbackup(1412): "graph"-Datei %s kann nicht ge?ffnet werden\n
413 fbackup(1413): Konfigurationsdatei %s kann nicht ge?ffnet werden\n
414 fbackup(1414): Ung?ltiger Konfigurationsdateikennsatz (%s) wurde igoriert\n
415 fbackup(1415): Fehler in der Konfigurationsdatei an Zeile %d\n
416 Falsche Zeile:\n%s\n
417 fbackup(1417): Datumsdatei %s kann nicht im Lesemodus ge?ffnet werden\n
418 fbackup(1418): Die letzte Session der Ebene %d f?r Graph %s\n
419 \tbegonnen\t: %s\n
420 \tbeendet  : %s\n
421 fbackup(1421): Es sind keine Aufzeichnungs-Informationen f?r die "graph"-Datei %s verf?gbar
422 (unterhalb Ebene %d)\n
423 fbackup(1423): Die Datumsdatei %s konnte nicht zum Beschreiben ge?ffnet werden\n
424 fbackup(1424): Die tempor?re Datumsdatei %s kann nicht ge?ffnet werden\n
425 BEGONNEN: %s
426 BEENDET: %s\n
427 fbackup(1427): Ung?ltiger Eintrag in "graph"-Datei %s\n
428 fbackup(1428): Fehler beim Erweitern des Ausgabedateimusters\n
429 fbackup(1429): Anzahl der Ausgabedateien darf nicht gr??er als %d sein\n
501 fbackup(1501): Status von %s konnte in Funktion search nicht ermittelt werden\n
502 fbackup(1502): Verzeichnis in Funktion search konnte nicht in %s ge?ndert werden.\n
503 fbackup(1503): Verzeichnis konnte nicht in Startverzeichnis %s ge?ndert werden\n
504 fbackup(1504): Verzeichnis `.' (%s) kann nicht ge?ffnet werden.\n
505 fbackup(1505): Nicht gen?gend virtueller Speicher\n
506 fbackup(1506): Dateistatus von %s kann nicht ermittelt werden\n
507 fbackup(1507): Das Verzeichnis kann nicht zu %s ge?ndert werden\n
508 fbackup(1508): "cp2" kann in expanddir() nicht wieder angepa?t werden.\n
509 fbackup(1509): Initialisierungsfehler f?r findpath.\n
510 fbackup(1510): Status von %s kann nicht ermittelt werden\n
512 fbackup(1512): Nicht gen?gend virtueller Speicher\n
513 fbackup(1513): Fehler bei statcall in dotdotnotparent\n
514 fbackup(1514): Status von %s kann nicht ermittelt werden\n
515 fbackup(1515): Status von %s kann nicht ermittelt werden\n
516 fbackup(1516): %s nicht gesichert - Option 'n' (NFS) nicht angegeben \n
517 fbackup(1517): %s nicht gesichert - Option 'n' (NFS) nicht angegeben \n
601 fbackup(1601): Es kann nicht auf die -i/-e-Datei %s zugegriffen werden\n
602 fbackup(1602): Nicht gen?gend virtueller Speicher\n
603 fbackup(1603): Nicht gen?gend virtueller Speicher\n
604 fbackup(1604): Nicht gen?gend virtueller Speicher\n
605 fbackup(1605): Nicht gen?gend virtueller Speicher\n
701  <LINK>
702 fbackup(1702): Nicht gen?gend virtueller Speicher\n
704 fbackup(1704): Nicht gen?gend virtueller Speicher\n
705 fbackup(1705): Online-Indexdatei %s konnte nicht erstellt werden\n
706 fbackup(1706): Name der Neustart-Datei (R?cklauftaste zum Verlassen)\n
707 fbackup(1707): Erstellen der Neustart-Datei %s f?r Schreibvorg?nge nicht m?glich.\n
708 fbackup(1708): ?ffnen der Neustart-Datei %s f?r Lesevorg?nge nicht m?glich.\n
709 fbackup(1709): Nicht gen?gend virtueller Speicher\n
710 fbackup(1710): Nicht gen?gend virtueller Speicher\n
712 fbackup(1712): Neugestartete Session begann am %s\n
713 fbackup(1713): "graph"-Datei %s auf Ebene %d\n
714 fbackup(1714): Fortsetzung bei Dateinummer %d auf Datentr?ger %d\n
715 fbackup(1715): Datei %s f?r Kopfinformationen zu Datentr?gern kann nicht erstellt werden\n
801 fbackup(1801): Nicht gen?gend virtueller Speicher\n
802 fbackup(1802): Nicht gen?gend virtueller Speicher\n
$set 2
1 fbackup(2001): reader %d: shmat f?r Speichersegment nicht erfolgreich\n
2 fbackup(2002): reader %d: Datei %s kann nicht ge?ffnet werden\n
3 fbackup(2003): reader %d: lseek f?r Datei %s nicht m?glich\n
4 fbackup(2004): reader %d: Erster Lesefehler,
5 fbackup(2005): reader %d: Zweiter Lesefehler,
6 fbackup(2006): reader %d: Es wurde ein Signal empfangen, ohne da? ein Arbeitschritt durchgef?hrt wurde\n
7 fbackup(2007): reader %d: Arbeit wird abgebrochen\n
8 fbackup(2008): reader %d: Datei %s kann nicht gelesen werden,\n  Datei ist gesperrt\n
$set 3
1 fbackup(3001): shmat nicht erfolgreich f?r das Schreiben des Speichersegmentes; Fehlernummer: %d\n
2 fbackup(3002): Nicht gen?gend virtueller Speicher\n
3 fbackup(3003): Normales EOT\n
4 fbackup(3004): Schreibvorgang bricht ab\n
5 fbackup(3005): WARNUNG: Datei mit der Nummer %d wurde NICHT gesichert.\n
6 fbackup(3006): Keine korrekte Abstimmung der Schreibvorg?nge, fno=%d, filenum=%d\n
7 fbackup(3007): WARNUNG: Dateinummer %d (%s)\n
8 \twar aktiv w?hrend des Versuchs mit der Nummer %d\n
9 fbackup(3009): WARNUNG: Dateinummer %d (%s)\n
10 \twurde nicht erfolgreich gesichert\n
11 fbackup(3011): Schreibfehler w?hrend des Schreibens eines checkpoint-EOF,
12 fbackup(3012): Schreibfehler w?hrend des Schreibens eines checkpoint-Satzes,
13 fbackup(3013): Schreibfehler w?hrend des Schreibens eines Datensatzes,
14 fbackup(3014): Schreibfehler: Es konnte nicht in die Ausgabedatei geschrieben werden\n
15 fbackup(3015): blockorientierte Ausgabeger?te sind nicht unterst?tzt\n
16 fbackup(3016): R?cklauftaste dr?cken, wenn Datentr?ger %d auf %s bereit ist?\n
17 fbackup(3017): Band ist nicht am Anfang, es wird zur?ckgespult\n
18 fbackup(3018): Typ der Ausgabedatei %s ist ung?ltig\n
19 fbackup(3019): Wollen Sie eine neue Ausgabedatei angeben?\n
20 fbackup(3020): Bitte den Namen der neuen Datei angeben:\n
21 fbackup(3021): Nicht gen?gend virtueller Speicher\n
22 fbackup(3022): Wollen Sie diese Session fortsetzen?\n
23 fbackup(3023): Vergewissern Sie sich, da? Datentr?ger %d auf %s bereit ist\n
24 fbackup(3024): Datentr?ger %d wird in Ausgabedatei %s geschrieben\n
25 fbackup(3025): Schreibfehler am Anfang des Datentr?gers\n
26 fbackup(3026): ioctl-Fehler, Ausgabedatei kann nicht abgefragt werden\n
27 fbackup(3027): ioctl-Fehler, Ausgabedatei kann nicht abgefragt werden\n
28 fbackup(3028): Schreibfehler beim Schreiben eines Pr?f-EOFs,
29 fbackup(3029): Schreibfehler beim Schreiben eines Pr?fsatzes,
30 fbackup(3030): Schreibfehler beim Schreiben eines Datensatzes,
32 fbackup(3032): Ausgabedatei %s kann nicht ge?ffnet werden\n
33 fbackup(3033): Typ dieser Ausgabedatei ist ung?ltig; bitte geben Sie ein
34 Ger?t im UCB-Modus ohne R?ckspulen\noder ein Ger?t im AT&T-Modus mit 
35 R?ckspulen an\n
36 fbackup(3036): Datentr?ger %s konnte nicht auf den Anfang positioniert werden\n
37 fbackup(3037): Ausgabedatei %s konnte nicht ge?ffnet werden\n
43 fbackup(3043): Schreibfehler beim Schreiben eines Datensatzes,
44 fbackup(3044): ioctl-Fehler, Ausgabedatei kann nicht abgefragt werden\n
45 fbackup(3045): Schreibfehler beim Schreiben eines Datensatzes,
46 fbackup(3046): R?cklauftaste dr?cken, wenn Datentr?ger %d auf %s bereit ist?\n
47 fbackup(3047): Ausgabedatei %s konnte nicht ge?ffnet werden\n
48 fbackup(3048): Datentr?ger %s konnte nicht auf den Anfang positioniert werden\n
49 fbackup(3049): R?cklauftaste dr?cken, wenn Datentr?ger %d auf %s bereit ist?\n
50 fbackup(3050): Ausgabedatei %s konnte nicht ge?ffnet werden\n
51 fbackup(3051): R?cklauftaste dr?cken, wenn Datentr?ger %d auf %s bereit ist?\n
52 fbackup(3052): Ausgabedatei %s konnte nicht ge?ffnet werden\n
54 fbackup(3054): Ausgabedatei %s konnte nicht ge?ffnet werden\n
101  auf Datentr?gersatz %d\n
102 fbackup(3102): Versuch, diesen Datentr?ger wiederherstellbar zu machen\n
103 fbackup(3103): Neuschreiben des Datentr?gers von Beginn an?\n
104 fbackup(3104): Es werden 2 EOFs geschrieben, Band wird zur?ckgespult\n
105 fbackup(3105): Es werden 2 EOFs geschrieben, Band wird zur?ckgespult\n
106 fbackup(3106): Bitte legen Sie ein neues Band ein\n
107 fbackup(3107): Fehler beim Schreiben auf ein EOF in %s\n
108 fbackup(3108): Fehler beim Schreiben auf einen Satz in %s\n
109 fbackup(3109): Fehler beim Schreiben auf ein EOF in %s\n
110 fbackup(3110): Fehler beim Schreiben auf einen Satz in %s\n
111 fbackup(3111): Fehler beim Schreiben auf einen Satz in %s\n
112 fbackup(3112): Fehler beim Schreiben auf ein EOF in %s\n
113 fbackup(3113): Fehler beim Schreiben auf einen Satz in %s\n
114 fbackup(3114): Fehler beim Schreiben auf ein EOF in %s\n
115 fbackup(3115): Fehler beim Schreiben auf einen Satz in %s\n
116 fbackup(3116): Fehler beim Schreiben auf einen Satz in %s\n
117 fbackup(3117): Fehler beim Schreiben auf einen Satz in %s\n
118 fbackup(3118): Fehler beim Schreiben auf einen Satz in %s\n
119 fbackup(3119): Fehler beim Schreiben auf einen Satz in %s\n
120 fbackup(3120): Fehler beim Schreiben auf einen Satz in %s\n
121 fbackup(3121): Fehler beim Schreiben auf einen Satz in %s\n
122 fbackup(3122): Fehler beim Schreiben auf einen Satz in %s\n
123 fbackup(3123): Vorheriger Pr?fsatz konnte nicht gelesen werden\n
124 fbackup(3124): Fehler beim Schreiben auf einen Satz in %s\n
125 fbackup(3125): Fehler beim Schreiben auf ein FSM in %s\n
126 fbackup(3126): Fehler beim Schreiben auf einen Satz in %s\n
127 fbackup(3127): Fehler beim Schreiben auf einen Satz in %s\n
128 fbackup(3128): Fehler beim Schreiben auf ein EOF in %s\n
129 fbackup(3129): Fehler beim Schreiben auf einen Satz in %s\n
130 fbackup(3130): Fehler beim Schreiben auf einen Satz in %s\n
202 fbackup(3202): Das ist der Datentr?ger %d DIESER SITZUNG!\n\tDatentr?ger zur?ckgewiesen\n
203 fbackup(3203): Datentr?ger %d wurde %d Mal benutzt\n
204 fbackup(3204): Datentr?ger ?fter als das Maximum benutzt, wollen Sie diesen Datentr?ger trotzdem benutzen?\n
205 fbackup(3205): Datentr?gerkopf kann nicht gelesen werden\n
301 fbackup(3301): Ausgabedatei %s kann nicht ge?ffnet werden\n
303 fbackup(3303): ioctl-Fehler, Ausgabedatei kann nicht abgefragt werden.\n
304 fbackup(3304): Fehler bei Magnetbandoperation\n
305 fbackup(3305): Ausgabedatei %s kann nicht ge?ffnet werden\n
306 fbackup(3306): Das ist der Datentr?ger %d DIESER SITZUNG!\n\tDatentr?ger zur?ckgewiesen\n
307 fbackup(3307): Datentr?ger %d wurde %d mal benutzt (Maximum: %d)\n
308 fbackup(3308): M?chten Sie diesen Datentr?ger trotzdem benutzen?\n
309 fbackup(3309): Datentr?gerkopf kann nicht gelesen werden\n
310 fbackup(3310): Geben Sie '%s' ein, wenn Datentr?ger %d auf %s bereit ist,\n oder '%s', um abzubrechen:\n
$set 4
1 fbackup(4001): automatisches '%s'\n
2 fbackup(4002): Bitte antworten Sie mit: '%s' oder '%s'\n
3 fbackup(4003): Semaphorenoperation fehlgeschlagen: op=%d\n
4 fbackup(4004): Die Funktion mymalloc kann keinen Platz reservieren\n
