$set 1
1 Die -x Option ist nicht verf?gbar\n
2 Unbekannte Option %s\n
3 [Dr?cken Sie die R?cklauftaste, um fortzufahren]
101 Syntax: ed1000 [-r] [Datei]\n
$set 2
1 Addr1 > addr2|Erste Adresse ist gr??er als die zweite
2 Falscher Z?hler|Z?hler ungleich Null erforderlich
3 Adresse nicht erlaubt|Keine Adressangabe bei diesem Befehl erlaubt
4 Fehlerhaft aufgebaute Adresse
5 Suche bis zum Ende (BOTTOM) nicht erfolgreich|Suche bis zum Ende ohne ?bereinstimmende Muster
6 Suche bis zum Anfang (TOP) nicht erfolgreich|Suche bis zum Beginn ohne ?bereinstimmende Muster
7 Fehler|Muster nicht gefunden
8 Markierungen sind ' und a-z
9 Nichtdefinierte Markierung|Unbekannte Markierung nicht angesprochen
10 Negative Adresse|Negative Adresse - erste Pufferzeile ist 1
11 Es existieren nicht so viele Zeilen|Es existieren nicht so viele Zeilen im Puffer
$set 3
1 Abstand au?erhalb der erlaubten Grenzen|Abstandsangabe nach dem Befehl zu gro?
2 Heimverzeichnis unbekannt
3 Was soll markiert werden?|%s verlangt nachfolgenden Buchstaben
4 Falsche Markierung|Markierung mu? einen Buchstaben enthalten
5 Schutzvorgang nicht erfolgreich!
6 Datei gesch?tzt
7 Kein Schreibvorgang|Kein Schreibvorgang seit letzter ?nderung (:rewind! ?berschreibt)
8  HP-Version %s
9 Falsches Register
10 EOF erreicht|Am Dateiende amgelangt
11 Wie bitte?|Unbekanntes Befehlszeichen '%c'
12 Keine Jobsteuerung|Sie benutzen keine Shell mit Jobsteuerung
101  Ed1000 : 
102  Verwenden Sie das ?-Zeichen, wenn Sie die Hilfeeinrichtung nutzen wollen\n
$set 4
1 Zus?tzliche Zeichen|Zus?tzliche Zeichen am Ende des Befehls
2  [Warnung - %s ist unvollst?ndig]  \n                           \nIhre ?nderungen gehen verloren, wenn Sie keinen erfolgreichen Schreibvorgang\ndurchgef?hrt haben, bevor Sie die Session beenden.  Wenn Sie auf Ihre\nOriginaldatei ausgegeben haben, so wurden die vorherigen Inhalte\ndieser Datei zerst?rt. Nehmen Sie mit Ihrem Systemverwalter Kontakt\nauf, BEVOR Sie die Session beenden, falls Sie Unterst?tzung ben?tigen.\n
3 %d Dateien|%d Dateien zu bearbeiten
4 Keine weiteren Dateien|Keine weiteren Dateien zum Bearbeiten
5 Zusatzzeichen|Zusatzzeichen am Ende des "%s"-Befehls
6 %d weitere Dateien|%d weitere Dateien zum Bearbeiten
7 Kein Schreibvorgang|Kein Schreibvorgang seit letzter ?nderung (:%s! ?berschreibt)
8 Wie bitte?|%s: Kein solcher Befehl von open/visual
9 Wie bitte?|%s: Kein Editor-Befehl
10 [Dr?cken Sie die R?cklauftaste, um fortzufahren] 
$set 5
1 Nicht gen?gend Speicherplatz|Nicht gen?gend Speicherplatz - zu viele Zeilen in der Datei
2 Zeilen?berlauf|Ergebniszeile bei der Vereinigung ist zu lang
3 %s befindet sich wo?|%s verlangt eine nachfolgende Adressangabe
4 Dieses Umstellen w?rde nichts bewirken!
5 Umstellen zu einer bereits umgestellten Zeile
6 Einarbeitung in global/macro nicht m?glich
7 Zeile zu lang|Resultierende Zeile nach Umstellen ggf. zu lang
8 Falscher Bezeichner|Angabe eines Bezeichners pro Zeile
9 Kein vorheriger Bezeichner
10 %s: Falscher Eintrag in der Bezeichnerdatei
11 Kein Schreibvorgang|Kein Schreibvorgang seit letzter ?nderung (:tag! ?berschreibt)
12 Keine Bezeichnerdatei
13 %s: Bezeicher existiert nicht|%s: Bezeichner existiert nicht in Bezeichnerdatei
14 Arbeit innerhalb von global/macro nicht m?glich
15 \nAm Dateiende (EOF)
16 Am Dateiende (EOF)
17 Dr?cken Sie BOTTOM
18 Dr?cken Sie TOP
19 R?ckg?ngigmachen in globalen Befehlen nicht m?glich|R?ckg?ngigmachen in globalen Befehlen nicht m?glich
20 Es kann nichts r?ckg?ngig gemacht werden
21 Nichts ver?ndert|Letzter Befehl ver?nderte nichts
22 lhs fehlt
23 rhs fehlt
24 rhs fehlt
25 No tail recursion
26 Es ist zu gef?hrlich dies einzuordnen
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
4  [Ver?ndert]
5 Keine Datei
6  Zeile %d von %d --%ld%%--
7 Muster zu lang
8 ?berlauf des Argumentpuffers
9 Kein Alternativdateiname|Kein Alternativdateiname als Ersatz f?r #
10 Kein aktueller Dateiname|Kein aktueller Dateiname als Ersatz f?r %%
11 Pipe zu globalem Ausdruck nicht m?glich
12 Verzweigung zu globalem Ausdruck nicht m?glich
13 Argumentliste zu lang
14 Argumentliste zu lang
15 Keine ?bereinstimmung
16 Fehlender Dateiname
17 Nicht eindeutig|Zu viele Dateinamen
18 Dateiname zu lang
19  [Neue Datei]
20  Block-Ger?tedatei
21  Teletyp
22  Zeichen-Ger?tedatei
23  Verzeichnis
24  Ausf?hrbare Datei
25  Umstellbare Datei
26  Archivdatei
27  [Nur lesen]
28 Schreibformate sind 'w' und 'w>>'
29 Keine Datei|Keine aktueller Dateiname
30  Datei existiert| Datei existiert - verwenden Sie "w! %s" zum ?berschreiben
31  Datei kann nur gelesen werden
32  Datei kann nur gelesen werden
33  Verwenden Sie "w!",  um teilweise gef?llten Puffer zu schreiben
34  [Neue Datei]
35  [Existierende Datei]
36  [Unvollst?ndige letzte Zeile]
37  Zeile zu lang
38 Zu viele verschachtelte Quellen
39  %d Zeilen, %D Zeichen
40  (%D NULL)
101  Datei existiert| Datei existiert - verwenden Sie "wr or er %s" zum ?berschreiben
102  Verwenden Sie "wr", um teilweise gef?llten Puffer zu schreiben
$set 8
1 Globale Angabe innerhalb eines globalen Ausdrucks|Globale Angabe innerhalb eines globalen Ausdrucks nicht erlaubt
2 Globale Struktur ben?tigt regul?ren Ausdruck|Fehlender regul?rer Ausdruck f?r globale Struktur
3 Globaler Befehl zu lang
4 Ersatzschleife
5 Fehler|Ersetzen von Zeichenmustern fehlgeschlagen
6 Substitution ben?tigt regul?ren Ausdruck|Fehlender regul?rer Ausdruck f?r Substitution
7 Kein vorheriger regul?rer Ausdruck|Kein vorheriger regul?rer Ausdruck
8 Keine vorherige Substitution|Keine vorherige Substitution zu wiederholen
9 Ersatzmuster zu lang|Ersatzmuster zu lang - maximal 256 Zeichen
10 Zeilen?berlauf|Zeilen?berlauf bei Substitution
11 %d Substitutionen|%d Substitutionen
12  in %d Zeilen
13 Regul?re Ausdrucke k?nnen nicht durch Buchstaben oder Ziffern getrennt werden
14 Kein vorheriger Suchvorgang|Kein vorheriges Durchsuchen von regl?ren Ausdr?cken
15 Keine vorherige Substitution regul?rer Ausdr?cke|Keine vorherige Substitution regul?rer Ausdr?cke
16 Falsch aufgebauter regul?rer Ausdruck|Regul?rem Ausdruck \\ mu? ein / oder ? folgen
17 Kein vorheriger regul?rer Ausdruck|Kein vorheriger regul?rer Ausdruck
19 Regul?rer Ausdruck zu komplex|Regul?rer Ausdruck zu kompliziert
20 Nichtzutreffendes \\( oder zus?tzliches \\)|Ungleiche Anzahl von \\( und \\) in regul?rem Ausdruck
21 Zu viele \\(!|Zu viele \\(-Subausdr?cke in einem regul?ren Ausdruck
22 Falsches Vergleichselement|Ein Nicht-Vergleichselement wurde angegeben, wo ein solches erwartet wird
25 Falsches Bereichsende|Erster Begrenzung > zweite oder ctype-Klasse wurde als Endpunkt verwendet
26 Falsche Zeichenklasse|Ung?ltige ctype-Zeichenklasse '[: :]' angegeben
27 Falsche Pr?fliste|Leere oder unvollst?ndige Pr?fliste: '[]' oder '[^]'
29 Keine newline-Zeichen in regul?ren Ausdr?cken|Eingabe von newline-Zeichen in regul?ren Ausdr?cken nicht m?glich
30 Nachfolgende \\|Nachfolgende \\ in regul?ren Ausdr?cken nicht m?glich
32 Syntaxfehler in regul?rem Ausdruck|Ung?ltige Syntax f?r regul?ren Ausdruck verwendet
$set 9
1 %s: Option existiert nicht|%s: Option existiert nicht - 'set all' liefert alle g?ltigen Optionen
2 Option %s hat keine Umschaltfunktion
3 Fehlendes = |Fehlendes =-Zeichen in Zuweisung f?r Option %s 
4 Ziffern erwartet|Ziffern m?ssen angegeben werden nach = 
5 Zeichenkette zu lang|Zeichenkette in Optionszuweisung lang
6 ?nderung des Terminaltyps aus open/visual heraus nicht m?glich
$set 10
1 %d Zeilen
2 Adresse ungleich Null erforderlich|F?r diesen Befehl ist eine Adresse ungleich Null erforderlich
3 Keine Zeilen|Keine Zeilen im Puffer
4 %d Zeilen mehr|%d Zeilen mehr in der Datei nach %s
5 %d Zeilen weniger|%d Zeilen weniger in der Datei nach %s
6 Nicht gen?gend Speicher|Nicht gen?gend Speicher f?r undo - verwenden Sie ed
44 Systemfehler %d
45 emt trap, _ovno ist %d |emt trap, _ovno ist %d   - erneuter Versuch
46 \r\nIhre Datei wurde gesichert\r\n
47 \nUnterbrechung
$set 11
1  Tempor?re Datei zu gro?
2  Tempor?re Datei zu gro?
3 Nicht gen?gend Registerbereich
4 Kein Inhalt in Register %c
5 Eingabe einer unvollst?ndigen Zeile in ein Makro nicht m?glich
6 Kein Inhalt in Register %c
7 Register zu lang|Register zu lang, um in den Speicher eingepa?t werden zu k?nnen
$set 12
1 %s: Unbekannter Terminaltyp
$set 13
1 Unvollst?ndiger Shell-Escape-Befehl|Unvollst?ndiger Shell-Escape-Befehl - verwenden Sie 'shell', um eine Shell zu erhalten
2 Befehl zu lang
3 Kein vorheriger Befehl|Kein vorheriger Befehl, um ! zu ersetzen.
4 Kein Alternativdateiname|Kein Alternativdateiname, um # zu ersetzen.
5 Kein Dateiname|Kein Dateiname, um %% zu ersetzen
6 [Kein Schreibvorgang]|[Kein Schreibvorgang seit letzter ?nderung]
7 Kein vorheriger Befehl|Kein vorheriger Befehl zu wiederholen
8 Nutzen einer Pipe zum Filtern nicht m?glich
9 Keine weiteren Prozesse
10 Kein(e) %s!\n
11 Nutzen der Pipe nicht m?glich
12 Keine weiteren Prozesse
13  Nutzung einer Pipe zum Wiederherstellen nicht m?glich
14  Verzweigung zum Wiederherstellen nicht m?glich
15  Keine Wiederherstellungsprozedur
16 %d: Beendet mit Signal %d
17  -- Hauptspeicherabzug ausgegeben
$set 14
1 Fehler|Muster nicht an angegebener Zeile gefunden
2 Rekursive open/visual nicht erlaubt
3 Auf welchem Typ von Terminal arbeiten Sie - es ist nur '%s' bekannt
4 [Verwende open-Modus]
5 Der visuelle Modus verlangt adressierbaren Cursor oder upline-F?higkeit
6 Visueller Modus auf einem Bildschirm mit overstrike nicht m?glich
7 Der visuelle Modus verlangt die M?glichkeit zum Leeren des Bildschirminhalts
8 Der visuelle Modus verlangt die F?higkeit des Bildschirmrollens
9 Bildschirminhalt zu gro? f?r den internen Puffer
10 Es ist zu wenig ?ber Ihr Terminal bekannt, um %s nutzen zu k?nnen
11 Bildschirm zu breit
12 Bildschirminhalt zu gro?
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
3 Zeile zu lang zum ?ffnen
4 Zeile zu lang
$set 20
1 (NULL-Zeiger)
