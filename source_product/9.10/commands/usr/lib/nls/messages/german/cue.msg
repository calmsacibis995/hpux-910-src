$ @(#) $Header: cue.msg,v 70.2 92/06/16 18:19:01 ssa Exp $
$******************************************************************************
$
$ Hewlett-Packard Character-terminal User Environment (CUE)
$ Copyright (c) 1991 Hewlett-Packard Company
$
$ FILE: cue.msg
$
$ DESCRIPTION:
$       Contains all screen text for the CUE product.
$       Used to generate the American English, C, and n-computer
$       HP NLS message catalog for CUE (cue.cat).
$
$ NOTES:
$       To generate cue.cat from this file enter the following command:
$               gencat cue.cat cue.msg 
$
$ WARNING:
$       DO NOT USE message numbers above 1000 except in $set2, because
$       the message numbers in set2 MUST be unique.  Messages in set2
$       are used in the cuesession script like this:
$               /usr/bin/dumpmsg /usr/lib/nls/$LANG/cue.cat | grep 1003
$       Thus, if there is more than 1 message with this message number
$       the grep would print both messages in the script.
$
$******************************************************************************
$ 
$ General comments which apply to all messages:
$   All messages should be no wider than 75 columns so that users can
$ easily view them. Wider messages will still work, but will go beyond 
$ the right edge of the display and the users will have to use horizontal 
$ scrolling to read a message.
$ 
$set1 Core Screen Text
1 Anmeldefenster
2 Hewlett-Packard
3 Character-terminal User Environment (CUE)
4 Benutzername
5 Kennwort
$ 
$ Do not translate the & character in any of the following messages. 
$ Dialog Manager uses this notation to designate the character that 
$ usres enter to signal their item selection from a menu, instead of using 
$ the arrow keys.  For example the next entry (msg #6) instructs DM to 
$ enable the user enter character C to select Configuration item from the
$ main menu instead of using the arrow keys to highlite their selection.
$ 
6 &Konfiguration
7 &Sprache f?r die Session w?hlen
8 &Typ der zu startenden Session w?hlen
9 &Hilfe
10 &Einf?hrung in CUE
11 ?ndern der &Sprache Ihrer Session
12 ?ndern des &Typs Ihrer Session
13 Session sta&rten
14 Session &beenden
$ 
$ The following comments apply to all messages marked as "FUNCTION KEY LABLE".
$ The extra spaces in these messages is needed for the proper alignment of 
$ function key lables. For example message 15 is a function key label
$ such that End appears on the first line and Session on the second line
$ (directly below End) of the function key label.  In general, each function
$ key lable can have two lines. Each line in the label is 8 columns wide.
$ 
$ The following two messages (15 and 16) are FUNCTION KEY LABELS.
$ 
15 Session beenden
16  MEN?-   LEISTE
17 Sprachmen?
18 &OK
19 &L?schen
20 Sessionmen?
21 tsm   - Terminal Session Manager
22 keysh - Einfache, kontextgekoppelte Funktionstasten-Shell
23 sh    - Bourne Shell
24 csh   - C Shell
25 ksh   - Korn Shell
26 Fenster f?r Fehlermeldungen
27 Fenster f?r Hilfe
28 &Felder l?schen
$ 
$ The following two messages (29 and 30) are FUNCTION KEY LABELS.
$ 
29 Session starten
30  Felder  l?schen
31 Bitte w?hlen Sie die Sprache f?r den Bildschirmtext.
$ 
$ The following two messages (32 and 33) are displayed together at once;
$ in other words, they should be treated as one message. The translation
$ should fit in two lines.
$ 
32 Positionieren Sie den Cursor mit den Pfeiltasten und treffen Sie Ihre Wahl
33 mit der R?cklauftaste. Abschlie?end dr?cken Sie die Funktionstaste OK (f8).
34 Bitte w?hlen Sie den Typ f?r Ihre Session.
$
$ The following two messages (35 and 36) are FUNCTION KEY LABELS.
$
35    OK
36 L?schen
$
$ In the following message (37), the "<==" character sequence is a simple
$ pointing mechanism. It can be omitted from a translation, if it would make
$ it too complicated to understand. Also, a local version of a pointing
$ mechanism can be used instead, as long as the width of the message does
$ not exceed the 75-column limit.
$
37 <== diese Eingabe wird nicht angezeigt
$
$ Exclamation marks (!) are optional in the following message (38).
$ They are used for emphasis. They can be omitted or translated to a
$ suitable local character for that purpose.
$
38 Vergessen Sie nicht, Ihre Session zu beenden !!!
$
$ The following message (39) can be completely translated, including the
$ colon (:) character.
$
39 Willkommen,
40 Mit der Funktionstaste "MEN?LEISTE" (f4) k?nnen Sie die Elemente ausw?hlen.
41 Mit den Pfeiltasten (und Umschalttaste) k?nnen Sie weitere Daten anzeigen.
42 Dr?cken Sie die Funktionstaste OK (f8), wenn Sie fertig sind.
$ The following two messages (43 and 44) are FUNCTION KEY LABELS.
43  Hilfe
44  W?hlen

$set2 Messages from the script, cuesession
$ Please leave the punctuation as it is.
1001 Bitte warten Sie, w?hrend Ihre Session gestartet wird - 
1002 Der gew?hlte Session-Typ kann nicht gestartet werden - 
1003 CUE-Warnung: Heimverzeichnis ist "/".
1004 Zum Verlassen geben Sie "exit" ein oder dr?cken Sie: KTRL+D.
1005 Um TSM zu verlassen, rufen Sie mit (f8) das Men? "Ende" auf.

$
$ Messages in the following set are in the same format as they would appear
$ on the screen. The general format is of each message is:
$   <cat1> :
$   <cat2> :
$   <cat3> :
$   <cat4> :
$ wher <cat1> represents a category; such as Name, or Purpose.
$ Note that each category (Name, Purpose, Invoke, or any other) should start on
$ a new line. Trnaslations for each category can be separated into lines by
$ using "\n" character sequence.  In such cases, starting with the second line,
$ enough blanks are prepended so that the left edge is lined up and indented
$ (look at the format of Purpose section for message 1 below as an example).
$
$set3 Help Messages
1 Name   : OK\n\
Zweck  : Erm?glicht dem Benutzer, nach der gew?nschten Auswahl\n\
         zum vorherigen Fenster zur?ckzukehren.\n\
Aufruf : R?cklauftaste oder Funktionstaste "W?hlen" (f3) dr?cken\n\
Weiter : Mit Tab- oder Pfeiltasten zu einem anderen Feld.

2 Name   : L?schen\n\
Zweck  : Aktuelle Wahl r?ckg?ngig machen und zu der Wahl zur?ck-\n\
         kehren, die vor Aufruf dieses Men?s g?ltig war.\n\
Aufruf : R?cklauftaste oder Funktionstaste "W?hlen" (f3) dr?cken\n\
Weiter : Mit Tab- oder Pfeiltasten zu einem anderen Feld.

3 Name   : Benutzername\n\
Zweck  : Benutzername hier eingeben\n\
Weiter : Mit Tab- oder Pfeiltasten zu einem anderen Feld\n\
         oder mit MEN?LEISTE eine Men?leistenauswahl treffen.\n\
Details:\n\
Geben Sie Ihren Benutzernamen (maximal 8 Zeichen) ein.\n\
Eingabefehler k?nnen Sie mit der R?cktaste oder mit\n\
der Funktionstaste "Felder l?schen" korrigieren; dabei\n\
werden sowohl Benutzername als auch Kennworteingabe\n\
gel?scht.\n\n\
Nach Abschlu? der Eingabe springen Sie mit der R?cklauftaste, der\n\
Tab-Taste oder einer Pfeiltaste zum n?chsten Feld oder dr?cken Sie\n\
die Funktionstaste MEN?LEISTE (f4), um eine Men?leistenauswahl\n\
zu treffen.

4 Name   : Kennwort\n\
Zweck  : Kennwort hier eingeben\n\
Aktion : Um eine Session zu starten, dr?cken Sie nach Eingabe\n\
         des Kennworts die R?cklauftaste.\n\
Weiter : Mit Tab- oder Pfeiltasten zu einem anderen Feld\n\
         oder mit MEN?LEISTE eine Men?leistenauswahl treffen.\n\
Details:\n\
Geben Sie Ihr Kennwort ein. Die hierbei eingegebenen Zeichen\n\
werden aus Gr?nden der Datensicherheit nicht angezeigt.\n\
Eingabefehler k?nnen Sie mit der R?cktaste oder mit\n\
der Funktionstaste "Felder l?schen" korrigieren; dabei\n\
werden sowohl Benutzername als auch Kennworteingabe\n\
gel?scht.\n\n\
Pfeiltaste zum n?chsten Feld, oder Sie dr?cken die R?cklauftaste,\n\
um die Session zu starten, oder Sie dr?cken die Funktionstaste\n\
MEN?LEISTE (f4), um eine Men?leistenauswahl zu treffen.

5 Name   : Session starten\n\
Zweck  : CUE pr?ft Benutzername und Kennwort, bevor die Session\n\
         gestartet wird.\n\
Aufruf : R?cklauftaste oder Funktionstaste "W?hlen" (f3) dr?cken\n\
Weiter : Mit Tab- oder Pfeiltasten zu einem anderen Feld\n\
         oder mit MEN?LEISTE eine Men?leistenauswahl treffen.\n\
Details:\n\
Nachden Sie den Benutzernamen und das Kenntwort eingegeben und\n\
wahlweise eine Men?leistenauswahl getroffen haben, k?nnen Sie die\n\
Session starten. Um dieses Tastenfeld auszuw?hlen, dr?cken Sie die\n\
R?cklauftaste oder die Funktionstaste "W?hlen" (f3). Benutzername\n\
und Kennwort werden daraufhin gepr?ft. Wenn beide Eingaben korrekt\n\
sind, wird die Session gestartet. Sind die Eingaben falsch, so\n\
k?nnen Sie Benutzernamen und Kennwort erneut eingeben. Mit der\n\
Tab-Taste oder den Pfeiltasten gelangen Sie zum Feld f?r den\n\
Benutzernamen zur?ck. Sie k?nnen auch die Funktionstaste\n\
MEN?LEISTE (f4) dr?cken, um eine Men?leistenauswahl zu treffen.

6 Name   : Felder l?schen\n\
Zweck  : L?scht die Eingaben f?r Benutzername und Kennwort.\n\
Aufruf : R?cklauftaste oder Funktionstaste "W?hlen" (f3) dr?cken\n\
Weiter : Mit Tab- oder Pfeiltasten zu einem anderen Feld\n\
         oder mit MEN?LEISTE eine Men?leistenauswahl treffen.

7 Name   : Session beenden\n\
Zweck  : Beenden der CUE-Anmeldung/Abmeldung\n\
Aufruf : R?cklauftaste oder Funktionstaste "W?hlen" (f3) dr?cken\n\
Weiter : Mit Tab- oder Pfeiltasten zu einem anderen Feld\n\
         oder mit MEN?LEISTE eine Men?leistenauswahl treffen.

8 Name   : Sprachmen?\n\
Zweck  : Mit diesem Men? k?nnen Sie die Sprache f?r diese\n\
         Bildschirme und f?r Ihre Session w?hlen.\n\
Aufruf : R?cklauftaste oder Funktionstaste "W?hlen" (f3) dr?cken\n\
Weiter : Mit den Pfeiltasten zu einem anderen Element der Men?leiste\n\
         oder die Men?leiste durch erneutes Dr?cken von MEN?LEISTE\n\
         verlassen.\n\
Standard:Sofern Sie mit CUE keine andere Sprache gew?hlt haben,\n\
         wird standardm??ig die Sprache C verwendet.\n\
Details: Ihre Umbegungsvariable $LANG wird entsprechend Ihrer Wahl\n\
         ge?ndert.

9 Name   : Sessiontyp-Menu\n\
Zweck  : In diesem Men? k?nnen Sie den Typ Ihrer Session w?hlen.\n\
Aufruf : R?cklauftaste oder Funktionstaste "W?hlen" (f3) dr?cken\n\
Weiter : Mit den Pfeiltasten zu einem anderen Element der Men?leiste\n\
         oder die Men?leiste durch erneutes Dr?cken von MEN?LEISTE\n\
         verlassen.\n\
Standard:Sofern Sie mit CUE keinen anderen Session-Typ gew?hlt haben,\n\
         wird standardm??ig die Bourne Shell - /bin/sh verwendet.

10 Name   : Einf?hrung in CUE\n\
Zweck  : Aufruf von Hilfetexten zum allgemeinen Gebrauch von CUE, zum\n\
         Beispiel zum Starten einer Session oder zur Verwendung der\n\
         Men?s.\n\
Aufruf : R?cklauftaste oder Funktionstaste "W?hlen" (f3) dr?cken\n\
Weiter : Mit den Pfeiltasten zu einem anderen Feld der Men?leiste\n\
         oder die Men?leiste durch erneutes Dr?cken von MEN?LEISTE\n\
         verlassen.

11 Name   : Sprachwahl\n\
Zweck  : Mit diesem Men? kann f?r die Bildschirme und Texte eine\n\
         andere Sprache gew?hlt werden.\n\
Auswahl: Setzen Sie den Cursor mit den Pfeiltasten bzw. Umschalttaste\n\
         und Pfeil (f?r vertikales Rollen) oder den Tasten\n\
         "Seite ^/Seite v" (f?r horizontales Rollen) auf die ge-\n\
         w?nschte Sprache. Dann dr?cken Sie die R?cklauftaste oder die\n\
         Funktionstaste "W?hlen", um Ihre Wahl zu best?tigen.\n\
Standard:Sofern Sie mit CUE keine andere Sprache gew?hlt haben,\n\
         wird standardm??ig die Sprache C verwendet.\n\
Details: Ihre Umbegungsvariable $LANG wird entsprechend Ihrer Wahl\n\
         ge?ndert.\n\
Zur?ck : Dr?cken Sie die Funktionstaste OK (f8) oder setzen Sie den\n\
         Cursor mit der Tab-Taste auf das Tastenfeld "OK" und dr?cken\n\
         Sie die R?cklauftaste, um die Sprache zu aktivieren und zum\n\
         vorherigen Fenster zur?ckzukehren.

$
$ Maintain formating by observing the number of space characters
$ in the Details portion of message 12.
$
12 Name   : Sessiontyp-Wahl\n\
Zweck  : Wahl eines anderen Typs f?r Ihre Session.\n\
Auswahl: Setzen Sie den Cursor mit den Pfeiltasten bzw. Umschalttaste\n\
         und Pfeil (f?r vertikales Rollen) oder den Tasten\n\
         "Seite ^/Seite v" (f?r horizontales Rollen) auf den ge-\n\
         w?nschten Session-Typ. Dann dr?cken Sie die R?cklauftaste\n\
         oder die Funktionstaste "W?hlen", um Ihre Wahl zu best?tigen.\n\
Zur?ck : Dr?cken Sie die Funktionstaste OK (f8) oder setzen Sie den\n\
         Cursor mit der Tab-Taste auf das Tastenfeld "OK" und dr?cken\n\
         Sie die R?cklauftaste, um den Session-Typ zu aktivieren und\n\
         zum vorherigen Fenster zur?ckzukehren.\n\
Details: Nachdem Sie sich angemeldet haben, k?nnen Sie durch\n\
folgende Eingabe weitere Informationen ?ber den Session-Typ\n\
aufrufen: man <Session-Typ>.\n\
Um beispielsweise mehr ?ber "sh" zu erfahren, geben Sie\n\
folgendes ein:\n\
         man sh\n\n\
Session-Typ     Beschreibung\n\
____________    _____________________________________________\n\
sh              Die Bourne Shell ist die Standard-Programmiersprache\n\
                oder "Shell", die Befehle von einem Terminal oder\n\
                aus einer Datei ausf?hrt. Diese Shell wird als\n\
                standardm??iger Session-Typ verwendet, sofern Sie\n\
                keinen anderen Session-Typ w?hlen.\n\n\
ksh             Die Korn Shell (ksh) erweitert den Funktionsumfang der\n\
                sh um Funktionen wie z.B. Protokollieren und Editieren\n\
                von Befehlen, Vergabe von Alias-Namen und Verwendung\n\
                von Bedingungsausdr?cken.\n\n\
keysh           Die Keysh ist eine Erweiterung der Korn Shell um eine\n\
                benutzerfreundliche Men?f?hrung. Dabei unterst?tzen\n\
                hierarchische Funktionstastenmen?s und kontext-\n\
                gekoppelte Hilfetexte den Benutzer beim Erstellen von\n\
                Befehlszeilen. Die Keysh ist ausschlie?lich daten-\n\
                gesteuert und erlaubt es, die Men?s und Hilfetexte mit\n\
                wenig Aufwand beliebig zu erweitern. Die Keysh wird\n\
                insbesondere f?r das Erlernen des Betriebssystems\n\
                HP-UX empfohlen.\n\n\
csh             Die C Shell (csh) ist ein Befehlsspracheninterpreter\n\
                mit einem Befehlsprotokollpuffer, einer C-?hnlichen\n\
                Syntax und Jobsteuerungsfunktionen.\n\n\
tsm             Terminal Sessions Manager (TSM) ist ein Fenster-System\n\
                f?r ASCII-Terminals. TSM bietet schnellen Zugriff auf\n\
                bis zu 10 Computersessions, ?hnlich wie Shell-Layers\n\
                shl(1), beh?lt dabei aber den visuellen Kontext\n\
                und den Terminal-Status jeder Session bei.\n\
                Jede unter TSM laufende Session steuert das gesamte\n\
                Terminal und hat Zugriff auf den gesamten Bildschirm.\n\
                Wenn Sie mit HP-UX vertraut sind, kann TSM sehr\n\
                n?tzlich sein.

13 Name   : Hilfetext-Wahl\n\
Zweck  : Anzeigen von Hilfetexten.\n\
Anzeige: Nicht angezeigte Hilfetexte k?nnen mit den Pfeiltasten bzw.\n\
         Umschalttaste und Pfeil (f?r vertikales Rollen) oder den\n\
         Tasten "Seite ^/Seite v" (f?r horizontales Rollen) auf den\n\
         Bildschirm gerufen werden.\n\
Zur?ck : Dr?cken Sie die Funktionstaste OK (f8) oder setzen Sie den\n\
         Cursor mit der Tab-Taste auf das Feld "OK" und dr?cken Sie\n\
         die R?cklauftaste, um zum vorherigen Fenster zur?ckzukehren.

14 Name   : Fehlermeldungen\n\
Zweck  : Anzeigen von Fehlermeldungen.\n\
Anzeige: Nicht angezeigte Fehlermeldungen k?nnen mit den Pfeiltasten\n\
         bzw. Umschalttaste und Pfeil (f?r vertikales Rollen) oder den\n\
         Tasten "Seite ^/Seite v" (f?r horizontales Rollen) auf den\n\
         Bildschirm gerufen werden.\n\
Zur?ck : Dr?cken Sie die Funktionstaste OK (f8) oder setzen Sie den\n\
         Cursor mit der Tab-Taste auf das Feld "OK" und dr?cken Sie\n\
         die R?cklauftaste, um zum vorherigen Fenster zur?ckzukehren.
$
$ Maintain format by preserving the extra spaces so that the message
$ is lined up in the Screen Navigation portion:
$
15 Name   : Einf?hrung in CUE\n\
Zweck  : ?bersicht ?ber die Benutzung von CUE\n\
Anzeige: Nicht angezeigte Hilfetexte k?nnen mit den Pfeiltasten bzw.\n\
         Umschalttaste und Pfeil (f?r vertikales Rollen) oder den\n\
         Tasten "Seite ^/Seite v" (f?r horizontales Rollen) auf den\n\
         Bildschirm gerufen werden.\n\
Details:\n\
         Die Hauptfunktion von CUE besteht darin, die Anmeldung\n\
         am System sowie den Start der angepa?ten Session\n\
         m?glichst einfach zu gestalten. Praxisgerechte Bildschirme,\n\
         ?bersichtliche Men?s und Hilfefenster leisten Hilfestellung\n\
         beim Anpassen und Start der Session.\n\
Hilfe aufrufen:\n\
         Wenn Sie weitere Informationen zu einem Feld, einem\n\
         Tastenfeld oder einem Men?-Element w?nschen, dr?cken\n\
         Sie einfach die Funktionstaste "Hilfe" (f1).\n\
Anpassen der Session:\n\
         Bevor Sie Ihren Benutzernamen und Ihr Kennwort eingeben,\n\
         k?nnen Sie f?r Ihre CUE-Session folgende Einstellungen\n\
         w?hlen:\n\
            o Die Sprache, in der die Texte auf dem Bildschirm\n\
              erscheinen\n\
            o Den Typ f?r die Session, den Sie f?r Ihre Session\n\
              benutzen m?chten, nachdem Sie angemeldet sind\n\
         Um diese Einstellungen an Ihre Erfordernisse anzupassen,\n\
         dr?cken Sie die Funktionstaste MEN?LEISTE (f4) und w?hlen Sie\n\
         "Konfiguration". Um die Men?leiste zu verlassen, dr?cken Sie\n\
         erneut die Funktionstaste MEN?LEISTE.\n\
Starten der Session:\n\
         Nachdem Sie die Session angepa?t haben, k?nnen Sie Ihren\n\
         Benutzernamen und Ihr Kennwort eingeben. Wenn Sie im\n\
         Kennwortfeld die R?cklauftaste dr?cken, werden Benutzername\n\
         und Kennwort mit denen des Computers verglichen.\n\
         Verl?uft die Pr?fung erfolgreich, wird die Session gestartet,\n\
         falls nicht, k?nnen Sie Ihren Benutzernamen und Ihr Kennwort\n\
         erneut eingeben.\n\
Bildschirm-Wegweiser:\n\
         Dieser Abschnitt beschreibt, wie Sie sich innerhalb der\n\
         CUE-Bildschirme bewegen k?nnen.\n\n\
         Tasten:\n\
            Pfeil auf/ab        - zeilenweise bewegen\n\
            Pfeil links/rechts  - spaltenweise bewegen\n\
            Tab                 - Sprung zum n?chsten Feld\n\
            Umschalttaste\n\
            +Pfeil auf/ab       - eine Seite nach oben/unten rollen\n\
            LEERTASTE           - Auswahl treffen\n\
            R?cklauftaste       - Eingabe oder Wahl best?tigen\n\
            R?cktaste           - Zeichen l?schen\n\
            Seite v     - horizontal um eine Seite nach rechts rollen\n\
            Seite ^     - horizontal um eine Seite nach links rollen\n\
            Bildschirm l?schen - erneuert die Anzeige des gesamten\n\
                                 Bildschirms\n\
         Allgemeine Funktionstasten (f1-f8):\n\
            Hilfe (f1) - Mit dieser Funktionstaste kann jederzeit\n\
                         Hilfe zu dem Begriff unter dem Cursor auf-\n\
                         gerufen werden.\n\
            W?hlen (f3) - Mit dieser Funktionstaste kann ein Tasten-\n\
                          feld, ein Men?-Element oder ein Element in\n\
                          einer Liste ausgew?hlt werden.\n\
            MEN?LEISTE (f4) - Mit dieser Funktionstaste werden die\n\
                              Elemente der Men?leiste zur Auswahl\n\
                              angeboten.\n\
         Funktionstasten im Anmeldefenster (f1-f8):\n\
            Session (f5) - Mit dieser Funktionstaste werden Benutzer-\n\
            starten        name und Kennwort best?tigt; dann wird die\n\
                           Session gestartet.\n\
            Felder (f6) - Mit dieser Funktionstaste werden die Ein-\n\
            l?schen       gaben f?r Benutzername und Kennwort ge-\n\
                          l?scht.\n\
            Session (f8) - Mit dieser Funktionstaste wird die CUE-\n\
            beenden        Session beendet (Abmeldung).\n\
         Funktionstasten f?r Sprach-/Session-Fenster (f1-f8):\n\
            L?schen (f5) - Mit dieser Funktionstaste wird die aktuelle\n\
                           Eingabe gel?scht und die zuvor bei Anzeige\n\
                           dieses Men?s geltende Einstellung\n\
                           wiederhergestellt.\n\
            OK (f8) - Mit dieser Funktionstaste best?tigen Sie Ihre\n\
                      Wahl und kehren zum Anmeldefenster zur?ck.\n\
         Funktionstasten f?r Hilfe/Fehlermeldungen (f1-f8):\n\
            OK (f8) - Dr?cken Sie diese Funktionstaste, nachdem Sie\n\
                      die Meldungen gelesen haben.

16 Name   : ?ndern der Sprache f?r Ihre Session\n\
Zweck  : ?ndern der Sprache f?r Ihre Bildschirmanzeigen\n\
Details: W?hlen Sie durch Dr?cken der R?cklauftaste das Element\n\
         "Konfiguration" in der Men?leiste. Daraufhin erscheint ein\n\
         weiteres Men?, von dem aus Sie das Sprachmen? aufrufen\n\
         k?nnen.\n\
Zur?ck : Durch Dr?cken der Funktionstaste MEN?LEISTE (f4) k?nnen Sie\n\
         die Men?leiste verlassen.

17 Name   : ?ndern des Typs Ihrer Session\n\
Details: W?hlen Sie durch Dr?cken der R?cklauftaste das Element\n\
         "Konfiguration" in der Men?leiste. Daraufhin erscheint ein\n\
         weiteres Men?, in dem Sie den gew?nschten Typ f?r Ihre\n\
         Session ausw?hlen k?nnen.\n\
Zur?ck : Durch Dr?cken der Funktionstaste MEN?LEISTE (f4) k?nnen Sie\n\
         die Men?leiste verlassen.

18 Name   : ?ndern der Sprache f?r Ihre Session\n\
Zweck  : Liefert eine Anleitung zur Wahl einer anderen Sprache\n\
         f?r die CUE-Bildschirme und Ihre Session.\n\
Aufruf : R?cklauftaste oder Funktionstaste "W?hlen" (f3) dr?cken\n\
Weiter : Mit den Pfeiltasten zu einem anderen Element der Men?leiste\n\
         oder die Men?leiste durch erneutes Dr?cken von MEN?LEISTE\n\
         verlassen.\n\

19 Name   : ?ndern des Typs Ihrer Session\n\
Zweck  : Liefert eine Anleitung, wie Sie den Typ Ihrer Session ?ndern\n\
         k?nnen, die Sie w?nschen, nachdem Sie erfogreich Ihren\n\
         Benutzernamen und Ihr Kennwort eingegeben haben.\n\
Aufruf : R?cklauftaste oder Funktionstaste "W?hlen" (f3) dr?cken\n\
Weiter : Mit den Pfeiltasten zu einem anderen Element der Men?leiste\n\
         oder die Men?leiste durch erneutes Dr?cken von MEN?LEISTE\n\
         verlassen.\n\

$set4 NLS Language table must correlate with the nls_lang array in cue.c
$ DO NOT localize the NLS languages in parenthesis ()'s. Also, do NOT
$ change the order of entries.
$
1 ?hnlich STANDARD (C) mit Dollarzeichen $ (n-computer)
2 Amerikanisches Englisch (american)
3 Kanadisches Franz?sisch (c-french)
4 D?nisch
5 Niederl?ndisch
6 Britisches Englisch (english)
7 Finnisch
8 Franz?sisch
9 Deutsch
10 Italienisch
11 Norwegisch
12 Portugiesisch
13 Spanisch
14 Schwedisch
15 Isl?ndisch
16 Japanisch mit phonetischem Katakana-Alphabet (katakana)
17 Arabisch
18 Nordafrikanisches Arabisch ohne indische Zahlen (arabic-w)
19 Griechisch
20 Hebr?isch
21 T?rkisch
22 Thai
23 Von HP empfohlene STANDARD-Sprache, definiert in X/OPEN (C)
24 POSIX
25 Amerikanisches Englisch f?r Terminal nach ISO 8859/1 (american.iso88591)
26 Kanadisches Franz?sisch f?r Terminal nach ISO 8859/1 (c-french.iso88591)
27 D?nisch f?r Terminal nach ISO 8859/1 (danish.iso88591)
28 Niederl?ndisch f?r Terminal nach ISO 8859/1 (dutch.iso88591)
29 Britisches Englisch f?r Terminal nach ISO 8859/1 (english.iso88591)
30 Finnisch f?r Terminal nach ISO 8859/1 (finnish.iso88591)
31 Franz?sisch f?r Terminal nach ISO 8859/1 (french.iso88591)
32 Deutsch f?r Terminal nach ISO 8859/1 (german.iso88591)
33 Italienisch f?r Terminal nach ISO 8859/1 (italian.iso88591)
34 Norwegisch f?r Terminal nach ISO 8859/1 (norwegian.iso88591)
35 Portugiesisch f?r Terminal nach ISO 8859/1 (portuguese.iso88591)
36 Spanisch f?r Terminal nach ISO 8859/1 (spanish.iso88591)
37 Schwedisch f?r Terminal nach ISO 8859/1 (swedish.iso88591)
38 Isl?ndisch f?r Terminal nach ISO 8859/1 (icelandic.iso88591)
39 Tschechisch
40 Ungarisch
41 Polnisch
42 Rum?nisch
43 Serbokroatisch
44 Slowenisch
45 Russisch
46 Bulgarisch
47 Vereinfachtes Chinesisch, gebr?uchlich in VR China (chinese-s)
48 Traditionelles Chinesisch, gebr?uchlich in Taiwan (chinese-t)
49 Traditioelles Chinesisch, mit Gro?zeichensatz 5 (chinese-t.big5)
50 Japanisch mit Kanji-Zeichen und HP-15-Codierung (japanese)
51 Japanisch mit Kanji-Zeichen und EUC-Codierung (japanese.euc)
52 Koreanisch
53 Arabisch f?r Codeset ISO 88596 (arabic.iso88596)
54 Griechisch f?r Codeset ISO 88597 (greek.iso88597)
55 Hebr?isch f?r Codeset ISO 88598 (hebrew.iso88598)
56 T?rkisch f?r Codeset ISO 88599 (turkish.iso88599)

$set5 Error Messages
$
$ Strings of the form %<char> where <char> is a single character should
$ not be localized.
$ When multiple lines are present for a message, indent all lines after the
$ first line, so that all lines line up with the left edge of the first line.
$ Other extra spaces are also present for formating and therefore should be
$ preserved.
$
1 1 : CUE-Warnung.  Untersystem root : %s.
2 2 : CUE konnte den Meldungskatalog f?r die gew?nschte Sprache nicht\n\
    ?ffnen. CUE arbeitet mit der aktuellen Sprache weiter, aber Ihre\n\
    Session l?uft in der soeben gew?nschten Sprache ab.
3 3 : CUE-Warnung: Heimverzeichnis ist /.
4 4 : CUE Kann nicht in Ihr Heimverzeichnis %s wechseln.\n\
    Stellen Sie sicher, da? Ihr Heimverzeichniseintrag in der\n\
    Kennwort-Datei steht und zug?nglich ist.\n\
    Bei weiteren Fragen wenden Sie sich bitte an Ihren Systemverwalter.
5 5 : CUE Leider ist bereits die maximale Benutzeranzahl angemeldet.\n\
    Versuchen Sie es sp?ter noch einmal.
6 6 : CUE Kein Eintrag in /etc/utmp. Geben Sie ein: exec cue.
7 7 : CUE Kein Root-Verzeichnis f?r Unteranmeldung.\n\
    Wenden Sie sich bitte an Ihren Systemverwalter.
8 8 : CUE Kein /bin/cue auf root (/).\n\
    Wenden Sie sich bitte an Ihren Systemverwalter.
9 9 : CUE Ung?ltige numerische Gruppenkennung in der Datei passwd.\n\
    Wenden Sie sich bitte an Ihren Systemverwalter.
10 10 : CUE Ung?ltige numerische Benutzerkennung in der Datei passwd.\n\
    Wenden Sie sich bitte an Ihren Systemverwalter.
$
$ Preserve additional spaces for proper formating and line-up of the
$ message text.
$
11 11 : CUE Sie m?ssen Ihr Kennwort festlegen, weil Sie kein Kennwort\n\
haben oder Ihr Kennwort abgelaufen ist.\n\n\
Bei der Wahl des Kennworts sind folgende Regeln zu beachten:\n\
   * Das Kennwort mu? mindestens 6 Zeichen lang sein.\n\
     Nur die ersten 8 Zeichen werden ber?cksichtigt.\n\
   * Das Kennwort darf nicht mit dem Benutzernamen identisch sein\n\
     und darf auch nicht aus den gleichen Buchstaben zusammengesetzt\n\
     sein. Zwischen Gro?- und Kleinschreibung wird dabei nicht\n\
     unterschieden.\n\
   * Neue Kennw?rter m?ssen sich vom vorherigen Kennwort in mindestens\n\
     drei Zeichen unterscheiden. Zwischen Gro?- und Kleinschreibung\n\
     wird dabei nicht unterschieden.\n\
   * Mindestens 2 Zeichen m?ssen Buchstaben sein (Gro?- oder\n\
     Kleinbuchstaben).\n\
   * Mindestens ein Zeichen mu? eines der folgenden sein:\n\
      - Zahl (Ziffer 0 bis 9)\n\
      - Satz- oder Sonderzeichen (weder Buchstabe noch Zahl).\n\
        Beispiel: -, _ oder $)\n\n\
   Entsprechend dieser Regeln sind folgende Beispiele g?ltige Kennw?rter:\n\n\
      foo-bar   $money$   Number_9   @trophy\n\n\
   Au?erdem wird zwischen Gro?- und Kleinschreibung unterschieden.\n\
   Folgende Kennw?rter sind daher verschieden:\n\n\
      foo-bar   Foo-Bar   FOO-BAR\n\n\
   Das Kennwort ist so zu w?hlen, da? es sich als Ihr pers?nlicher,\n\
   geheimer Zugriffsschl?ssel eignet. Wer Ihr Kennwort kennt, kann sich\n\
   anmelden und auf Ihre Dateien zugreifen.\n\n\
   Folgende Schritte sind durchzuf?hren:\n\
   1. Falls Sie kein Kennwort haben, k?nnen Sie diesen Schritt auslassen.\n\
      Wenn Sie ein Kennwort haben, erscheint folgende Aufforderung:\n\n\
        Kennwort f?r Ihren Benutzernamen wird ge?ndert\n\
        Altes Kennwort:        Geben Sie Ihr aktuelles Kennwort ein\n\
   2. Geben Sie auf die folgende Aufforderung hin Ihr neues Kennwort\n\
      ein:\n\n\
        Neues Kennwort:\n\n\
   3. Sie werden aufgefordert, das neue Kennwort zur Best?tigung\n\
      erneut einzugeben:\n\n\
        Geben Sie Ihr neues Kennwort erneut ein:\n\n\
   4. Daraufhin erscheint erneut der CUE-Anmeldebildschirm,\n\
      in dem Sie sich mit Ihrem neuen Kennwort anmelden k?nnen.\n\n\
   Bei weiteren Fragen wenden Sie sich bitte an Ihren Systemverwalter.    
12 12 : CUE kann /bin/passwd nicht ausf?hren.\n\
     Wenden Sie sich bitte an Ihren Systemverwalter.
13 13 : CUE konnte "callbacks" nicht binden. Pr?fen Sie, ob CUE korrekt\n\
     installiert ist. Versuchen Sie es anschlie?end erneut.
14 14 : CUE kann /bin/cue.etc/cuesession nicht ausf?hren.\n\
     Wenden Sie sich bitte an Ihren Systemverwalter.
15 15 : CUE-Anmeldung fehlgeschlagen. Falsche Eingabe f?r Benutzername\n\
     oder Kennwort.
16 16 : CUE konnte Meldung Nr. %d aus Set %d nicht abrufen.\n\
     Pr?fen Sie, ob Ihre Umgebungsvariablen LANG und NLSPATH\n\
     zum NLS-Meldungskatalog cue.cat passen.\n\
     N?heres hierzu finden Sie in den Hilfeseiten environ(5) und lang(5).\n\
     Wenden Sie sich bitte an Ihren Systemverwalter.
17 17 : CUE steht nicht gen?gend Hauptspeicher zur Verf?gung.\n\
     Wenden Sie sich bitte an Ihren Systemverwalter.
18 18 : CUE kann Ihre Shell %s nicht starten.\n\
     Wenden Sie sich bitte an Ihren Systemverwalter.
19 19 : CUE konnte die Bildschirmdatei /bin/cue.etc/cue.dm nicht laden.\n\
     Pr?fen Sie, ob CUE korrekt installiert ist.\n
20 20 : CUE konnte den gew?schten Session-Typ im System nicht finden.\n\
     Bitte w?hlen Sie einen anderen Session-Typ.
21 21 : CUE kann die 'uid' nicht zur?cksetzen. Stellen Sie sicher, da?\n\
     das setuid-Bit f?r /bin/cue gesetzt ist. Wenden Sie sich bitte\n\
     an Ihren Systemverwalter.
22 22 : CUE kann /usr/lib/nls/config nicht ?ffnen. Wenden Sie sich\n\
     bitte an Ihren Systemverwalter.
23 23 : Eingeschr?nkte Shell. CUE kann nicht von Befehlszeile starten.
