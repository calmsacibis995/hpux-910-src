$set 1
1 Die -x Option ist nicht verfÏgbar\n
2 Unbekannte Option %s\n
3 [DrÏcken Sie die RÏcklauftaste, um fortzufahren]
101 Syntax: ed1000 [-r] [Datei]\n
$set 2
1 Addr1 > addr2|Erste Adresse ist grÎŞer als die zweite
2 Falscher ZÌhler|ZÌhler ungleich Null erforderlich
3 Adresse nicht erlaubt|Keine Adressangabe bei diesem Befehl erlaubt
4 Fehlerhaft aufgebaute Adresse
5 Suche bis zum Ende (BOTTOM) nicht erfolgreich|Suche bis zum Ende ohne Ïbereinstimmende Muster
6 Suche bis zum Anfang (TOP) nicht erfolgreich|Suche bis zum Beginn ohne Ïbereinstimmende Muster
7 Fehler|Muster nicht gefunden
8 Markierungen sind ' und a-z
9 Nichtdefinierte Markierung|Unbekannte Markierung nicht angesprochen
10 Negative Adresse|Negative Adresse - erste Pufferzeile ist 1
11 Es existieren nicht so viele Zeilen|Es existieren nicht so viele Zeilen im Puffer
$set 3
1 Abstand auŞerhalb der erlaubten Grenzen|Abstandsangabe nach dem Befehl zu groŞ
2 Heimverzeichnis unbekannt
3 Was soll markiert werden?|%s verlangt nachfolgenden Buchstaben
4 Falsche Markierung|Markierung muŞ einen Buchstaben enthalten
5 Schutzvorgang nicht erfolgreich!
6 Datei geschÏtzt
7 Kein Schreibvorgang|Kein Schreibvorgang seit letzter Ønderung (:rewind! Ïberschreibt)
8  HP-Version %s
9 Falsches Register
10 EOF erreicht|Am Dateiende amgelangt
11 Wie bitte?|Unbekanntes Befehlszeichen '%c'
12 Keine Jobsteuerung|Sie benutzen keine Shell mit Jobsteuerung
101  Ed1000 : 
102  Verwenden Sie das ?-Zeichen, wenn Sie die Hilfeeinrichtung nutzen wollen\n
$set 4
1 ZusÌtzliche Zeichen|ZusÌtzliche Zeichen am Ende des Befehls
2  [Warnung - %s ist unvollstÌndig]  \n                           \nIhre Ønderungen gehen verloren, wenn Sie keinen erfolgreichen Schreibvorgang\ndurchgefÏhrt haben, bevor Sie die Session beenden.  Wenn Sie auf Ihre\nOriginaldatei ausgegeben haben, so wurden die vorherigen Inhalte\ndieser Datei zerstÎrt. Nehmen Sie mit Ihrem Systemverwalter Kontakt\nauf, BEVOR Sie die Session beenden, falls Sie UnterstÏtzung benÎtigen.\n
3 %d Dateien|%d Dateien zu bearbeiten
4 Keine weiteren Dateien|Keine weiteren Dateien zum Bearbeiten
5 Zusatzzeichen|Zusatzzeichen am Ende des "%s"-Befehls
6 %d weitere Dateien|%d weitere Dateien zum Bearbeiten
7 Kein Schreibvorgang|Kein Schreibvorgang seit letzter Ønderung (:%s! Ïberschreibt)
8 Wie bitte?|%s: Kein solcher Befehl von open/visual
9 Wie bitte?|%s: Kein Editor-Befehl
10 [DrÏcken Sie die RÏcklauftaste, um fortzufahren] 
$set 5
1 Nicht genÏgend Speicherplatz|Nicht genÏgend Speicherplatz - zu viele Zeilen in der Datei
2 ZeilenÏberlauf|Ergebniszeile bei der Vereinigung ist zu lang
3 %s befindet sich wo?|%s verlangt eine nachfolgende Adressangabe
4 Dieses Umstellen wÏrde nichts bewirken!
5 Umstellen zu einer bereits umgestellten Zeile
6 Einarbeitung in global/macro nicht mÎglich
7 Zeile zu lang|Resultierende Zeile nach Umstellen ggf. zu lang
8 Falscher Bezeichner|Angabe eines Bezeichners pro Zeile
9 Kein vorheriger Bezeichner
10 %s: Falscher Eintrag in der Bezeichnerdatei
11 Kein Schreibvorgang|Kein Schreibvorgang seit letzter Ønderung (:tag! Ïberschreibt)
12 Keine Bezeichnerdatei
13 %s: Bezeicher existiert nicht|%s: Bezeichner existiert nicht in Bezeichnerdatei
14 Arbeit innerhalb von global/macro nicht mÎglich
15 \nAm Dateiende (EOF)
16 Am Dateiende (EOF)
17 DrÏcken Sie BOTTOM
18 DrÏcken Sie TOP
19 RÏckgÌngigmachen in globalen Befehlen nicht mÎglich|RÏckgÌngigmachen in globalen Befehlen nicht mÎglich
20 Es kann nichts rÏckgÌngig gemacht werden
21 Nichts verÌndert|Letzter Befehl verÌnderte nichts
22 lhs fehlt
23 rhs fehlt
24 rhs fehlt
25 No tail recursion
26 Es ist zu gefÌhrlich dies einzuordnen
27 No tail recursion
28 lhs fehlt
29 Nicht eingeordnet|Dieses Makro wurde nicht eingeordnet
30 Zu viele Makros
31 Zuviel Makrotext
$set 6
1 Eingabezeile zu lang
$set 7
1 Keine Datei|Kein aktueller Dateiname
2  [Nur Lesen]
3  [Nicht bearbeitet]
4  [VerÌndert]
5 Keine Datei
6  Zeile %d von %d --%ld%%--
7 Muster zu lang
8 Ûberlauf des Argumentpuffers
9 Kein Alternativdateiname|Kein Alternativdateiname als Ersatz fÏr #
10 Kein aktueller Dateiname|Kein aktueller Dateiname als Ersatz fÏr %%
11 Pipe zu globalem Ausdruck nicht mÎglich
12 Verzweigung zu globalem Ausdruck nicht mÎglich
13 Argumentliste zu lang
14 Argumentliste zu lang
15 Keine Ûbereinstimmung
16 Fehlender Dateiname
17 Nicht eindeutig|Zu viele Dateinamen
18 Dateiname zu lang
19  [Neue Datei]
20  Block-GerÌtedatei
21  Teletyp
22  Zeichen-GerÌtedatei
23  Verzeichnis
24  AusfÏhrbare Datei
25  Umstellbare Datei
26  Archivdatei
27  [Nur lesen]
28 Schreibformate sind 'w' und 'w>>'
29 Keine Datei|Keine aktueller Dateiname
30  Datei existiert| Datei existiert - verwenden Sie "w! %s" zum Ûberschreiben
31  Datei kann nur gelesen werden
32  Datei kann nur gelesen werden
33  Verwenden Sie "w!",  um teilweise gefÏllten Puffer zu schreiben
34  [Neue Datei]
35  [Existierende Datei]
36  [UnvollstÌndige letzte Zeile]
37  Zeile zu lang
38 Zu viele verschachtelte Quellen
39  %d Zeilen, %D Zeichen
40  (%D NULL)
101  Datei existiert| Datei existiert - verwenden Sie "wr or er %s" zum Ïberschreiben
102  Verwenden Sie "wr", um teilweise gefÏllten Puffer zu schreiben
$set 8
1 Globale Angabe innerhalb eines globalen Ausdrucks|Globale Angabe innerhalb eines globalen Ausdrucks nicht erlaubt
2 Globale Struktur benÎtigt regulÌren Ausdruck|Fehlender regulÌrer Ausdruck fÏr globale Struktur
3 Globaler Befehl zu lang
4 Ersatzschleife
5 Fehler|Ersetzen von Zeichenmustern fehlgeschlagen
6 Substitution benÎtigt regulÌren Ausdruck|Fehlender regulÌrer Ausdruck fÏr Substitution
7 Kein vorheriger regulÌrer Ausdruck|Kein vorheriger regulÌrer Ausdruck
8 Keine vorherige Substitution|Keine vorherige Substitution zu wiederholen
9 Ersatzmuster zu lang|Ersatzmuster zu lang - maximal 256 Zeichen
10 ZeilenÏberlauf|ZeilenÏberlauf bei Substitution
11 %d Substitutionen|%d Substitutionen
12  in %d Zeilen
13 RegulÌre Ausdrucke kÎnnen nicht durch Buchstaben oder Ziffern getrennt werden
14 Kein vorheriger Suchvorgang|Kein vorheriges Durchsuchen von reglÌren AusdrÏcken
15 Keine vorherige Substitution regulÌrer AusdrÏcke|Keine vorherige Substitution regulÌrer AusdrÏcke
16 Falsch aufgebauter regulÌrer Ausdruck|RegulÌrem Ausdruck \\ muŞ ein / oder ? folgen
17 Kein vorheriger regulÌrer Ausdruck|Kein vorheriger regulÌrer Ausdruck
19 RegulÌrer Ausdruck zu komplex|RegulÌrer Ausdruck zu kompliziert
20 Nichtzutreffendes \\( oder zusÌtzliches \\)|Ungleiche Anzahl von \\( und \\) in regulÌrem Ausdruck
21 Zu viele \\(!|Zu viele \\(-SubausdrÏcke in einem regulÌren Ausdruck
22 Falsches Vergleichselement|Ein Nicht-Vergleichselement wurde angegeben, wo ein solches erwartet wird
25 Falsches Bereichsende|Erster Begrenzung > zweite oder ctype-Klasse wurde als Endpunkt verwendet
26 Falsche Zeichenklasse|UngÏltige ctype-Zeichenklasse '[: :]' angegeben
27 Falsche PrÏfliste|Leere oder unvollstÌndige PrÏfliste: '[]' oder '[^]'
29 Keine newline-Zeichen in regulÌren AusdrÏcken|Eingabe von newline-Zeichen in regulÌren AusdrÏcken nicht mÎglich
30 Nachfolgende \\|Nachfolgende \\ in regulÌren AusdrÏcken nicht mÎglich
32 Syntaxfehler in regulÌrem Ausdruck|UngÏltige Syntax fÏr regulÌren Ausdruck verwendet
$set 9
1 %s: Option existiert nicht|%s: Option existiert nicht - 'set all' liefert alle gÏltigen Optionen
2 Option %s hat keine Umschaltfunktion
3 Fehlendes = |Fehlendes =-Zeichen in Zuweisung fÏr Option %s 
4 Ziffern erwartet|Ziffern mÏssen angegeben werden nach = 
5 Zeichenkette zu lang|Zeichenkette in Optionszuweisung lang
6 Ønderung des Terminaltyps aus open/visual heraus nicht mÎglich
$set 10
1 %d Zeilen
2 Adresse ungleich Null erforderlich|FÏr diesen Befehl ist eine Adresse ungleich Null erforderlich
3 Keine Zeilen|Keine Zeilen im Puffer
4 %d Zeilen mehr|%d Zeilen mehr in der Datei nach %s
5 %d Zeilen weniger|%d Zeilen weniger in der Datei nach %s
6 Nicht genÏgend Speicher|Nicht genÏgend Speicher fÏr undo - verwenden Sie ed
44 Systemfehler %d
45 emt trap, _ovno ist %d |emt trap, _ovno ist %d   - erneuter Versuch
46 \r\nIhre Datei wurde gesichert\r\n
47 \nUnterbrechung
$set 11
1  TemporÌre Datei zu groŞ
2  TemporÌre Datei zu groŞ
3 Nicht genÏgend Registerbereich
4 Kein Inhalt in Register %c
5 Eingabe einer unvollstÌndigen Zeile in ein Makro nicht mÎglich
6 Kein Inhalt in Register %c
7 Register zu lang|Register zu lang, um in den Speicher eingepaŞt werden zu kÎnnen
$set 12
1 %s: Unbekannter Terminaltyp
$set 13
1 UnvollstÌndiger Shell-Escape-Befehl|UnvollstÌndiger Shell-Escape-Befehl - verwenden Sie 'shell', um eine Shell zu erhalten
2 Befehl zu lang
3 Kein vorheriger Befehl|Kein vorheriger Befehl, um ! zu ersetzen.
4 Kein Alternativdateiname|Kein Alternativdateiname, um # zu ersetzen.
5 Kein Dateiname|Kein Dateiname, um %% zu ersetzen
6 [Kein Schreibvorgang]|[Kein Schreibvorgang seit letzter Ønderung]
7 Kein vorheriger Befehl|Kein vorheriger Befehl zu wiederholen
8 Nutzen einer Pipe zum Filtern nicht mÎglich
9 Keine weiteren Prozesse
10 Kein(e) %s!\n
11 Nutzen der Pipe nicht mÎglich
12 Keine weiteren Prozesse
13  Nutzung einer Pipe zum Wiederherstellen nicht mÎglich
14  Verzweigung zum Wiederherstellen nicht mÎglich
15  Keine Wiederherstellungsprozedur
16 %d: Beendet mit Signal %d
17  -- Hauptspeicherabzug ausgegeben
$set 14
1 Fehler|Muster nicht an angegebener Zeile gefunden
2 Rekursive open/visual nicht erlaubt
3 Auf welchem Typ von Terminal arbeiten Sie - es ist nur '%s' bekannt
4 [Verwende open-Modus]
5 Der visuelle Modus verlangt adressierbaren Cursor oder upline-FÌhigkeit
6 Visueller Modus auf einem Bildschirm mit overstrike nicht mÎglich
7 Der visuelle Modus verlangt die MÎglichkeit zum Leeren des Bildschirminhalts
8 Der visuelle Modus verlangt die FÌhigkeit des Bildschirmrollens
9 Bildschirminhalt zu groŞ fÏr den internen Puffer
10 Es ist zu wenig Ïber Ihr Terminal bekannt, um %s nutzen zu kÎnnen
11 Bildschirm zu breit
12 Bildschirminhalt zu groŞ
$set 15
1 Interner Fehler: vscroll
2 Keine Zeilen im Puffer
3 Interner Fehler: vredraw
$set 16
1 Eingabelesefehler
2 %d Zeilen mehr 
3 %d Zeilen weniger
4 %d Zeile
5 Makro zu lang|Makro zu lang  - vielleicht eine Rekursion?
$set 17
1 Unendliche Makroschleife
$set 18
1 Unendliche Makroschleife
2 Zeile zu lang
3 Zeile zu lang
$set 19
1 Interner Fehler: vclreol
2 Interner Fehler: vgoto
3 Zeile zu lang zum Úffnen
4 Zeile zu lang
$set 20
1 (NULL-Zeiger)
