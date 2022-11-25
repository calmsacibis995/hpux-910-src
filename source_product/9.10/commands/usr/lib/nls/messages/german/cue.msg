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
7 &Sprache fœr die Session wÃhlen
8 &Typ der zu startenden Session wÃhlen
9 &Hilfe
10 &Einfœhrung in CUE
11 ÿndern der &Sprache Ihrer Session
12 ÿndern des &Typs Ihrer Session
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
16  MEN€-   LEISTE
17 Sprachmenœ
18 &OK
19 &LŒschen
20 Sessionmenœ
21 tsm   - Terminal Session Manager
22 keysh - Einfache, kontextgekoppelte Funktionstasten-Shell
23 sh    - Bourne Shell
24 csh   - C Shell
25 ksh   - Korn Shell
26 Fenster fœr Fehlermeldungen
27 Fenster fœr Hilfe
28 &Felder lŒschen
$ 
$ The following two messages (29 and 30) are FUNCTION KEY LABELS.
$ 
29 Session starten
30  Felder  lŒschen
31 Bitte wÃhlen Sie die Sprache fœr den Bildschirmtext.
$ 
$ The following two messages (32 and 33) are displayed together at once;
$ in other words, they should be treated as one message. The translation
$ should fit in two lines.
$ 
32 Positionieren Sie den Cursor mit den Pfeiltasten und treffen Sie Ihre Wahl
33 mit der Rœcklauftaste. Abschlieﬁend drœcken Sie die Funktionstaste OK (f8).
34 Bitte wÃhlen Sie den Typ fœr Ihre Session.
$
$ The following two messages (35 and 36) are FUNCTION KEY LABELS.
$
35    OK
36 LŒschen
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
40 Mit der Funktionstaste "MEN€LEISTE" (f4) kŒnnen Sie die Elemente auswÃhlen.
41 Mit den Pfeiltasten (und Umschalttaste) kŒnnen Sie weitere Daten anzeigen.
42 Drœcken Sie die Funktionstaste OK (f8), wenn Sie fertig sind.
$ The following two messages (43 and 44) are FUNCTION KEY LABELS.
43  Hilfe
44  WÃhlen

$set2 Messages from the script, cuesession
$ Please leave the punctuation as it is.
1001 Bitte warten Sie, wÃhrend Ihre Session gestartet wird - 
1002 Der gewÃhlte Session-Typ kann nicht gestartet werden - 
1003 CUE-Warnung: Heimverzeichnis ist "/".
1004 Zum Verlassen geben Sie "exit" ein oder drœcken Sie: KTRL+D.
1005 Um TSM zu verlassen, rufen Sie mit (f8) das Menœ "Ende" auf.

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
Zweck  : ErmŒglicht dem Benutzer, nach der gewœnschten Auswahl\n\
         zum vorherigen Fenster zurœckzukehren.\n\
Aufruf : Rœcklauftaste oder Funktionstaste "WÃhlen" (f3) drœcken\n\
Weiter : Mit Tab- oder Pfeiltasten zu einem anderen Feld.

2 Name   : LŒschen\n\
Zweck  : Aktuelle Wahl rœckgÃngig machen und zu der Wahl zurœck-\n\
         kehren, die vor Aufruf dieses Menœs gœltig war.\n\
Aufruf : Rœcklauftaste oder Funktionstaste "WÃhlen" (f3) drœcken\n\
Weiter : Mit Tab- oder Pfeiltasten zu einem anderen Feld.

3 Name   : Benutzername\n\
Zweck  : Benutzername hier eingeben\n\
Weiter : Mit Tab- oder Pfeiltasten zu einem anderen Feld\n\
         oder mit MEN€LEISTE eine Menœleistenauswahl treffen.\n\
Details:\n\
Geben Sie Ihren Benutzernamen (maximal 8 Zeichen) ein.\n\
Eingabefehler kŒnnen Sie mit der Rœcktaste oder mit\n\
der Funktionstaste "Felder lŒschen" korrigieren; dabei\n\
werden sowohl Benutzername als auch Kennworteingabe\n\
gelŒscht.\n\n\
Nach Abschluﬁ der Eingabe springen Sie mit der Rœcklauftaste, der\n\
Tab-Taste oder einer Pfeiltaste zum nÃchsten Feld oder drœcken Sie\n\
die Funktionstaste MEN€LEISTE (f4), um eine Menœleistenauswahl\n\
zu treffen.

4 Name   : Kennwort\n\
Zweck  : Kennwort hier eingeben\n\
Aktion : Um eine Session zu starten, drœcken Sie nach Eingabe\n\
         des Kennworts die Rœcklauftaste.\n\
Weiter : Mit Tab- oder Pfeiltasten zu einem anderen Feld\n\
         oder mit MEN€LEISTE eine Menœleistenauswahl treffen.\n\
Details:\n\
Geben Sie Ihr Kennwort ein. Die hierbei eingegebenen Zeichen\n\
werden aus Grœnden der Datensicherheit nicht angezeigt.\n\
Eingabefehler kŒnnen Sie mit der Rœcktaste oder mit\n\
der Funktionstaste "Felder lŒschen" korrigieren; dabei\n\
werden sowohl Benutzername als auch Kennworteingabe\n\
gelŒscht.\n\n\
Pfeiltaste zum nÃchsten Feld, oder Sie drœcken die Rœcklauftaste,\n\
um die Session zu starten, oder Sie drœcken die Funktionstaste\n\
MEN€LEISTE (f4), um eine Menœleistenauswahl zu treffen.

5 Name   : Session starten\n\
Zweck  : CUE prœft Benutzername und Kennwort, bevor die Session\n\
         gestartet wird.\n\
Aufruf : Rœcklauftaste oder Funktionstaste "WÃhlen" (f3) drœcken\n\
Weiter : Mit Tab- oder Pfeiltasten zu einem anderen Feld\n\
         oder mit MEN€LEISTE eine Menœleistenauswahl treffen.\n\
Details:\n\
Nachden Sie den Benutzernamen und das Kenntwort eingegeben und\n\
wahlweise eine Menœleistenauswahl getroffen haben, kŒnnen Sie die\n\
Session starten. Um dieses Tastenfeld auszuwÃhlen, drœcken Sie die\n\
Rœcklauftaste oder die Funktionstaste "WÃhlen" (f3). Benutzername\n\
und Kennwort werden daraufhin geprœft. Wenn beide Eingaben korrekt\n\
sind, wird die Session gestartet. Sind die Eingaben falsch, so\n\
kŒnnen Sie Benutzernamen und Kennwort erneut eingeben. Mit der\n\
Tab-Taste oder den Pfeiltasten gelangen Sie zum Feld fœr den\n\
Benutzernamen zurœck. Sie kŒnnen auch die Funktionstaste\n\
MEN€LEISTE (f4) drœcken, um eine Menœleistenauswahl zu treffen.

6 Name   : Felder lŒschen\n\
Zweck  : LŒscht die Eingaben fœr Benutzername und Kennwort.\n\
Aufruf : Rœcklauftaste oder Funktionstaste "WÃhlen" (f3) drœcken\n\
Weiter : Mit Tab- oder Pfeiltasten zu einem anderen Feld\n\
         oder mit MEN€LEISTE eine Menœleistenauswahl treffen.

7 Name   : Session beenden\n\
Zweck  : Beenden der CUE-Anmeldung/Abmeldung\n\
Aufruf : Rœcklauftaste oder Funktionstaste "WÃhlen" (f3) drœcken\n\
Weiter : Mit Tab- oder Pfeiltasten zu einem anderen Feld\n\
         oder mit MEN€LEISTE eine Menœleistenauswahl treffen.

8 Name   : Sprachmenœ\n\
Zweck  : Mit diesem Menœ kŒnnen Sie die Sprache fœr diese\n\
         Bildschirme und fœr Ihre Session wÃhlen.\n\
Aufruf : Rœcklauftaste oder Funktionstaste "WÃhlen" (f3) drœcken\n\
Weiter : Mit den Pfeiltasten zu einem anderen Element der Menœleiste\n\
         oder die Menœleiste durch erneutes Drœcken von MEN€LEISTE\n\
         verlassen.\n\
Standard:Sofern Sie mit CUE keine andere Sprache gewÃhlt haben,\n\
         wird standardmÃﬁig die Sprache C verwendet.\n\
Details: Ihre Umbegungsvariable $LANG wird entsprechend Ihrer Wahl\n\
         geÃndert.

9 Name   : Sessiontyp-Menu\n\
Zweck  : In diesem Menœ kŒnnen Sie den Typ Ihrer Session wÃhlen.\n\
Aufruf : Rœcklauftaste oder Funktionstaste "WÃhlen" (f3) drœcken\n\
Weiter : Mit den Pfeiltasten zu einem anderen Element der Menœleiste\n\
         oder die Menœleiste durch erneutes Drœcken von MEN€LEISTE\n\
         verlassen.\n\
Standard:Sofern Sie mit CUE keinen anderen Session-Typ gewÃhlt haben,\n\
         wird standardmÃﬁig die Bourne Shell - /bin/sh verwendet.

10 Name   : Einfœhrung in CUE\n\
Zweck  : Aufruf von Hilfetexten zum allgemeinen Gebrauch von CUE, zum\n\
         Beispiel zum Starten einer Session oder zur Verwendung der\n\
         Menœs.\n\
Aufruf : Rœcklauftaste oder Funktionstaste "WÃhlen" (f3) drœcken\n\
Weiter : Mit den Pfeiltasten zu einem anderen Feld der Menœleiste\n\
         oder die Menœleiste durch erneutes Drœcken von MEN€LEISTE\n\
         verlassen.

11 Name   : Sprachwahl\n\
Zweck  : Mit diesem Menœ kann fœr die Bildschirme und Texte eine\n\
         andere Sprache gewÃhlt werden.\n\
Auswahl: Setzen Sie den Cursor mit den Pfeiltasten bzw. Umschalttaste\n\
         und Pfeil (fœr vertikales Rollen) oder den Tasten\n\
         "Seite ^/Seite v" (fœr horizontales Rollen) auf die ge-\n\
         wœnschte Sprache. Dann drœcken Sie die Rœcklauftaste oder die\n\
         Funktionstaste "WÃhlen", um Ihre Wahl zu bestÃtigen.\n\
Standard:Sofern Sie mit CUE keine andere Sprache gewÃhlt haben,\n\
         wird standardmÃﬁig die Sprache C verwendet.\n\
Details: Ihre Umbegungsvariable $LANG wird entsprechend Ihrer Wahl\n\
         geÃndert.\n\
Zurœck : Drœcken Sie die Funktionstaste OK (f8) oder setzen Sie den\n\
         Cursor mit der Tab-Taste auf das Tastenfeld "OK" und drœcken\n\
         Sie die Rœcklauftaste, um die Sprache zu aktivieren und zum\n\
         vorherigen Fenster zurœckzukehren.

$
$ Maintain formating by observing the number of space characters
$ in the Details portion of message 12.
$
12 Name   : Sessiontyp-Wahl\n\
Zweck  : Wahl eines anderen Typs fœr Ihre Session.\n\
Auswahl: Setzen Sie den Cursor mit den Pfeiltasten bzw. Umschalttaste\n\
         und Pfeil (fœr vertikales Rollen) oder den Tasten\n\
         "Seite ^/Seite v" (fœr horizontales Rollen) auf den ge-\n\
         wœnschten Session-Typ. Dann drœcken Sie die Rœcklauftaste\n\
         oder die Funktionstaste "WÃhlen", um Ihre Wahl zu bestÃtigen.\n\
Zurœck : Drœcken Sie die Funktionstaste OK (f8) oder setzen Sie den\n\
         Cursor mit der Tab-Taste auf das Tastenfeld "OK" und drœcken\n\
         Sie die Rœcklauftaste, um den Session-Typ zu aktivieren und\n\
         zum vorherigen Fenster zurœckzukehren.\n\
Details: Nachdem Sie sich angemeldet haben, kŒnnen Sie durch\n\
folgende Eingabe weitere Informationen œber den Session-Typ\n\
aufrufen: man <Session-Typ>.\n\
Um beispielsweise mehr œber "sh" zu erfahren, geben Sie\n\
folgendes ein:\n\
         man sh\n\n\
Session-Typ     Beschreibung\n\
____________    _____________________________________________\n\
sh              Die Bourne Shell ist die Standard-Programmiersprache\n\
                oder "Shell", die Befehle von einem Terminal oder\n\
                aus einer Datei ausfœhrt. Diese Shell wird als\n\
                standardmÃﬁiger Session-Typ verwendet, sofern Sie\n\
                keinen anderen Session-Typ wÃhlen.\n\n\
ksh             Die Korn Shell (ksh) erweitert den Funktionsumfang der\n\
                sh um Funktionen wie z.B. Protokollieren und Editieren\n\
                von Befehlen, Vergabe von Alias-Namen und Verwendung\n\
                von Bedingungsausdrœcken.\n\n\
keysh           Die Keysh ist eine Erweiterung der Korn Shell um eine\n\
                benutzerfreundliche Menœfœhrung. Dabei unterstœtzen\n\
                hierarchische Funktionstastenmenœs und kontext-\n\
                gekoppelte Hilfetexte den Benutzer beim Erstellen von\n\
                Befehlszeilen. Die Keysh ist ausschlieﬁlich daten-\n\
                gesteuert und erlaubt es, die Menœs und Hilfetexte mit\n\
                wenig Aufwand beliebig zu erweitern. Die Keysh wird\n\
                insbesondere fœr das Erlernen des Betriebssystems\n\
                HP-UX empfohlen.\n\n\
csh             Die C Shell (csh) ist ein Befehlsspracheninterpreter\n\
                mit einem Befehlsprotokollpuffer, einer C-Ãhnlichen\n\
                Syntax und Jobsteuerungsfunktionen.\n\n\
tsm             Terminal Sessions Manager (TSM) ist ein Fenster-System\n\
                fœr ASCII-Terminals. TSM bietet schnellen Zugriff auf\n\
                bis zu 10 Computersessions, Ãhnlich wie Shell-Layers\n\
                shl(1), behÃlt dabei aber den visuellen Kontext\n\
                und den Terminal-Status jeder Session bei.\n\
                Jede unter TSM laufende Session steuert das gesamte\n\
                Terminal und hat Zugriff auf den gesamten Bildschirm.\n\
                Wenn Sie mit HP-UX vertraut sind, kann TSM sehr\n\
                nœtzlich sein.

13 Name   : Hilfetext-Wahl\n\
Zweck  : Anzeigen von Hilfetexten.\n\
Anzeige: Nicht angezeigte Hilfetexte kŒnnen mit den Pfeiltasten bzw.\n\
         Umschalttaste und Pfeil (fœr vertikales Rollen) oder den\n\
         Tasten "Seite ^/Seite v" (fœr horizontales Rollen) auf den\n\
         Bildschirm gerufen werden.\n\
Zurœck : Drœcken Sie die Funktionstaste OK (f8) oder setzen Sie den\n\
         Cursor mit der Tab-Taste auf das Feld "OK" und drœcken Sie\n\
         die Rœcklauftaste, um zum vorherigen Fenster zurœckzukehren.

14 Name   : Fehlermeldungen\n\
Zweck  : Anzeigen von Fehlermeldungen.\n\
Anzeige: Nicht angezeigte Fehlermeldungen kŒnnen mit den Pfeiltasten\n\
         bzw. Umschalttaste und Pfeil (fœr vertikales Rollen) oder den\n\
         Tasten "Seite ^/Seite v" (fœr horizontales Rollen) auf den\n\
         Bildschirm gerufen werden.\n\
Zurœck : Drœcken Sie die Funktionstaste OK (f8) oder setzen Sie den\n\
         Cursor mit der Tab-Taste auf das Feld "OK" und drœcken Sie\n\
         die Rœcklauftaste, um zum vorherigen Fenster zurœckzukehren.
$
$ Maintain format by preserving the extra spaces so that the message
$ is lined up in the Screen Navigation portion:
$
15 Name   : Einfœhrung in CUE\n\
Zweck  : €bersicht œber die Benutzung von CUE\n\
Anzeige: Nicht angezeigte Hilfetexte kŒnnen mit den Pfeiltasten bzw.\n\
         Umschalttaste und Pfeil (fœr vertikales Rollen) oder den\n\
         Tasten "Seite ^/Seite v" (fœr horizontales Rollen) auf den\n\
         Bildschirm gerufen werden.\n\
Details:\n\
         Die Hauptfunktion von CUE besteht darin, die Anmeldung\n\
         am System sowie den Start der angepaﬁten Session\n\
         mŒglichst einfach zu gestalten. Praxisgerechte Bildschirme,\n\
         œbersichtliche Menœs und Hilfefenster leisten Hilfestellung\n\
         beim Anpassen und Start der Session.\n\
Hilfe aufrufen:\n\
         Wenn Sie weitere Informationen zu einem Feld, einem\n\
         Tastenfeld oder einem Menœ-Element wœnschen, drœcken\n\
         Sie einfach die Funktionstaste "Hilfe" (f1).\n\
Anpassen der Session:\n\
         Bevor Sie Ihren Benutzernamen und Ihr Kennwort eingeben,\n\
         kŒnnen Sie fœr Ihre CUE-Session folgende Einstellungen\n\
         wÃhlen:\n\
            o Die Sprache, in der die Texte auf dem Bildschirm\n\
              erscheinen\n\
            o Den Typ fœr die Session, den Sie fœr Ihre Session\n\
              benutzen mŒchten, nachdem Sie angemeldet sind\n\
         Um diese Einstellungen an Ihre Erfordernisse anzupassen,\n\
         drœcken Sie die Funktionstaste MEN€LEISTE (f4) und wÃhlen Sie\n\
         "Konfiguration". Um die Menœleiste zu verlassen, drœcken Sie\n\
         erneut die Funktionstaste MEN€LEISTE.\n\
Starten der Session:\n\
         Nachdem Sie die Session angepaﬁt haben, kŒnnen Sie Ihren\n\
         Benutzernamen und Ihr Kennwort eingeben. Wenn Sie im\n\
         Kennwortfeld die Rœcklauftaste drœcken, werden Benutzername\n\
         und Kennwort mit denen des Computers verglichen.\n\
         VerlÃuft die Prœfung erfolgreich, wird die Session gestartet,\n\
         falls nicht, kŒnnen Sie Ihren Benutzernamen und Ihr Kennwort\n\
         erneut eingeben.\n\
Bildschirm-Wegweiser:\n\
         Dieser Abschnitt beschreibt, wie Sie sich innerhalb der\n\
         CUE-Bildschirme bewegen kŒnnen.\n\n\
         Tasten:\n\
            Pfeil auf/ab        - zeilenweise bewegen\n\
            Pfeil links/rechts  - spaltenweise bewegen\n\
            Tab                 - Sprung zum nÃchsten Feld\n\
            Umschalttaste\n\
            +Pfeil auf/ab       - eine Seite nach oben/unten rollen\n\
            LEERTASTE           - Auswahl treffen\n\
            Rœcklauftaste       - Eingabe oder Wahl bestÃtigen\n\
            Rœcktaste           - Zeichen lŒschen\n\
            Seite v     - horizontal um eine Seite nach rechts rollen\n\
            Seite ^     - horizontal um eine Seite nach links rollen\n\
            Bildschirm lŒschen - erneuert die Anzeige des gesamten\n\
                                 Bildschirms\n\
         Allgemeine Funktionstasten (f1-f8):\n\
            Hilfe (f1) - Mit dieser Funktionstaste kann jederzeit\n\
                         Hilfe zu dem Begriff unter dem Cursor auf-\n\
                         gerufen werden.\n\
            WÃhlen (f3) - Mit dieser Funktionstaste kann ein Tasten-\n\
                          feld, ein Menœ-Element oder ein Element in\n\
                          einer Liste ausgewÃhlt werden.\n\
            MEN€LEISTE (f4) - Mit dieser Funktionstaste werden die\n\
                              Elemente der Menœleiste zur Auswahl\n\
                              angeboten.\n\
         Funktionstasten im Anmeldefenster (f1-f8):\n\
            Session (f5) - Mit dieser Funktionstaste werden Benutzer-\n\
            starten        name und Kennwort bestÃtigt; dann wird die\n\
                           Session gestartet.\n\
            Felder (f6) - Mit dieser Funktionstaste werden die Ein-\n\
            lŒschen       gaben fœr Benutzername und Kennwort ge-\n\
                          lŒscht.\n\
            Session (f8) - Mit dieser Funktionstaste wird die CUE-\n\
            beenden        Session beendet (Abmeldung).\n\
         Funktionstasten fœr Sprach-/Session-Fenster (f1-f8):\n\
            LŒschen (f5) - Mit dieser Funktionstaste wird die aktuelle\n\
                           Eingabe gelŒscht und die zuvor bei Anzeige\n\
                           dieses Menœs geltende Einstellung\n\
                           wiederhergestellt.\n\
            OK (f8) - Mit dieser Funktionstaste bestÃtigen Sie Ihre\n\
                      Wahl und kehren zum Anmeldefenster zurœck.\n\
         Funktionstasten fœr Hilfe/Fehlermeldungen (f1-f8):\n\
            OK (f8) - Drœcken Sie diese Funktionstaste, nachdem Sie\n\
                      die Meldungen gelesen haben.

16 Name   : ÿndern der Sprache fœr Ihre Session\n\
Zweck  : ÿndern der Sprache fœr Ihre Bildschirmanzeigen\n\
Details: WÃhlen Sie durch Drœcken der Rœcklauftaste das Element\n\
         "Konfiguration" in der Menœleiste. Daraufhin erscheint ein\n\
         weiteres Menœ, von dem aus Sie das Sprachmenœ aufrufen\n\
         kŒnnen.\n\
Zurœck : Durch Drœcken der Funktionstaste MEN€LEISTE (f4) kŒnnen Sie\n\
         die Menœleiste verlassen.

17 Name   : ÿndern des Typs Ihrer Session\n\
Details: WÃhlen Sie durch Drœcken der Rœcklauftaste das Element\n\
         "Konfiguration" in der Menœleiste. Daraufhin erscheint ein\n\
         weiteres Menœ, in dem Sie den gewœnschten Typ fœr Ihre\n\
         Session auswÃhlen kŒnnen.\n\
Zurœck : Durch Drœcken der Funktionstaste MEN€LEISTE (f4) kŒnnen Sie\n\
         die Menœleiste verlassen.

18 Name   : ÿndern der Sprache fœr Ihre Session\n\
Zweck  : Liefert eine Anleitung zur Wahl einer anderen Sprache\n\
         fœr die CUE-Bildschirme und Ihre Session.\n\
Aufruf : Rœcklauftaste oder Funktionstaste "WÃhlen" (f3) drœcken\n\
Weiter : Mit den Pfeiltasten zu einem anderen Element der Menœleiste\n\
         oder die Menœleiste durch erneutes Drœcken von MEN€LEISTE\n\
         verlassen.\n\

19 Name   : ÿndern des Typs Ihrer Session\n\
Zweck  : Liefert eine Anleitung, wie Sie den Typ Ihrer Session Ãndern\n\
         kŒnnen, die Sie wœnschen, nachdem Sie erfogreich Ihren\n\
         Benutzernamen und Ihr Kennwort eingegeben haben.\n\
Aufruf : Rœcklauftaste oder Funktionstaste "WÃhlen" (f3) drœcken\n\
Weiter : Mit den Pfeiltasten zu einem anderen Element der Menœleiste\n\
         oder die Menœleiste durch erneutes Drœcken von MEN€LEISTE\n\
         verlassen.\n\

$set4 NLS Language table must correlate with the nls_lang array in cue.c
$ DO NOT localize the NLS languages in parenthesis ()'s. Also, do NOT
$ change the order of entries.
$
1 ÿhnlich STANDARD (C) mit Dollarzeichen $ (n-computer)
2 Amerikanisches Englisch (american)
3 Kanadisches FranzŒsisch (c-french)
4 DÃnisch
5 NiederlÃndisch
6 Britisches Englisch (english)
7 Finnisch
8 FranzŒsisch
9 Deutsch
10 Italienisch
11 Norwegisch
12 Portugiesisch
13 Spanisch
14 Schwedisch
15 IslÃndisch
16 Japanisch mit phonetischem Katakana-Alphabet (katakana)
17 Arabisch
18 Nordafrikanisches Arabisch ohne indische Zahlen (arabic-w)
19 Griechisch
20 HebrÃisch
21 Tœrkisch
22 Thai
23 Von HP empfohlene STANDARD-Sprache, definiert in X/OPEN (C)
24 POSIX
25 Amerikanisches Englisch fœr Terminal nach ISO 8859/1 (american.iso88591)
26 Kanadisches FranzŒsisch fœr Terminal nach ISO 8859/1 (c-french.iso88591)
27 DÃnisch fœr Terminal nach ISO 8859/1 (danish.iso88591)
28 NiederlÃndisch fœr Terminal nach ISO 8859/1 (dutch.iso88591)
29 Britisches Englisch fœr Terminal nach ISO 8859/1 (english.iso88591)
30 Finnisch fœr Terminal nach ISO 8859/1 (finnish.iso88591)
31 FranzŒsisch fœr Terminal nach ISO 8859/1 (french.iso88591)
32 Deutsch fœr Terminal nach ISO 8859/1 (german.iso88591)
33 Italienisch fœr Terminal nach ISO 8859/1 (italian.iso88591)
34 Norwegisch fœr Terminal nach ISO 8859/1 (norwegian.iso88591)
35 Portugiesisch fœr Terminal nach ISO 8859/1 (portuguese.iso88591)
36 Spanisch fœr Terminal nach ISO 8859/1 (spanish.iso88591)
37 Schwedisch fœr Terminal nach ISO 8859/1 (swedish.iso88591)
38 IslÃndisch fœr Terminal nach ISO 8859/1 (icelandic.iso88591)
39 Tschechisch
40 Ungarisch
41 Polnisch
42 RumÃnisch
43 Serbokroatisch
44 Slowenisch
45 Russisch
46 Bulgarisch
47 Vereinfachtes Chinesisch, gebrÃuchlich in VR China (chinese-s)
48 Traditionelles Chinesisch, gebrÃuchlich in Taiwan (chinese-t)
49 Traditioelles Chinesisch, mit Groﬁzeichensatz 5 (chinese-t.big5)
50 Japanisch mit Kanji-Zeichen und HP-15-Codierung (japanese)
51 Japanisch mit Kanji-Zeichen und EUC-Codierung (japanese.euc)
52 Koreanisch
53 Arabisch fœr Codeset ISO 88596 (arabic.iso88596)
54 Griechisch fœr Codeset ISO 88597 (greek.iso88597)
55 HebrÃisch fœr Codeset ISO 88598 (hebrew.iso88598)
56 Tœrkisch fœr Codeset ISO 88599 (turkish.iso88599)

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
2 2 : CUE konnte den Meldungskatalog fœr die gewœnschte Sprache nicht\n\
    Œffnen. CUE arbeitet mit der aktuellen Sprache weiter, aber Ihre\n\
    Session lÃuft in der soeben gewœnschten Sprache ab.
3 3 : CUE-Warnung: Heimverzeichnis ist /.
4 4 : CUE Kann nicht in Ihr Heimverzeichnis %s wechseln.\n\
    Stellen Sie sicher, daﬁ Ihr Heimverzeichniseintrag in der\n\
    Kennwort-Datei steht und zugÃnglich ist.\n\
    Bei weiteren Fragen wenden Sie sich bitte an Ihren Systemverwalter.
5 5 : CUE Leider ist bereits die maximale Benutzeranzahl angemeldet.\n\
    Versuchen Sie es spÃter noch einmal.
6 6 : CUE Kein Eintrag in /etc/utmp. Geben Sie ein: exec cue.
7 7 : CUE Kein Root-Verzeichnis fœr Unteranmeldung.\n\
    Wenden Sie sich bitte an Ihren Systemverwalter.
8 8 : CUE Kein /bin/cue auf root (/).\n\
    Wenden Sie sich bitte an Ihren Systemverwalter.
9 9 : CUE Ungœltige numerische Gruppenkennung in der Datei passwd.\n\
    Wenden Sie sich bitte an Ihren Systemverwalter.
10 10 : CUE Ungœltige numerische Benutzerkennung in der Datei passwd.\n\
    Wenden Sie sich bitte an Ihren Systemverwalter.
$
$ Preserve additional spaces for proper formating and line-up of the
$ message text.
$
11 11 : CUE Sie mœssen Ihr Kennwort festlegen, weil Sie kein Kennwort\n\
haben oder Ihr Kennwort abgelaufen ist.\n\n\
Bei der Wahl des Kennworts sind folgende Regeln zu beachten:\n\
   * Das Kennwort muﬁ mindestens 6 Zeichen lang sein.\n\
     Nur die ersten 8 Zeichen werden berœcksichtigt.\n\
   * Das Kennwort darf nicht mit dem Benutzernamen identisch sein\n\
     und darf auch nicht aus den gleichen Buchstaben zusammengesetzt\n\
     sein. Zwischen Groﬁ- und Kleinschreibung wird dabei nicht\n\
     unterschieden.\n\
   * Neue KennwŒrter mœssen sich vom vorherigen Kennwort in mindestens\n\
     drei Zeichen unterscheiden. Zwischen Groﬁ- und Kleinschreibung\n\
     wird dabei nicht unterschieden.\n\
   * Mindestens 2 Zeichen mœssen Buchstaben sein (Groﬁ- oder\n\
     Kleinbuchstaben).\n\
   * Mindestens ein Zeichen muﬁ eines der folgenden sein:\n\
      - Zahl (Ziffer 0 bis 9)\n\
      - Satz- oder Sonderzeichen (weder Buchstabe noch Zahl).\n\
        Beispiel: -, _ oder $)\n\n\
   Entsprechend dieser Regeln sind folgende Beispiele gœltige KennwŒrter:\n\n\
      foo-bar   $money$   Number_9   @trophy\n\n\
   Auﬁerdem wird zwischen Groﬁ- und Kleinschreibung unterschieden.\n\
   Folgende KennwŒrter sind daher verschieden:\n\n\
      foo-bar   Foo-Bar   FOO-BAR\n\n\
   Das Kennwort ist so zu wÃhlen, daﬁ es sich als Ihr persŒnlicher,\n\
   geheimer Zugriffsschlœssel eignet. Wer Ihr Kennwort kennt, kann sich\n\
   anmelden und auf Ihre Dateien zugreifen.\n\n\
   Folgende Schritte sind durchzufœhren:\n\
   1. Falls Sie kein Kennwort haben, kŒnnen Sie diesen Schritt auslassen.\n\
      Wenn Sie ein Kennwort haben, erscheint folgende Aufforderung:\n\n\
        Kennwort fœr Ihren Benutzernamen wird geÃndert\n\
        Altes Kennwort:        Geben Sie Ihr aktuelles Kennwort ein\n\
   2. Geben Sie auf die folgende Aufforderung hin Ihr neues Kennwort\n\
      ein:\n\n\
        Neues Kennwort:\n\n\
   3. Sie werden aufgefordert, das neue Kennwort zur BestÃtigung\n\
      erneut einzugeben:\n\n\
        Geben Sie Ihr neues Kennwort erneut ein:\n\n\
   4. Daraufhin erscheint erneut der CUE-Anmeldebildschirm,\n\
      in dem Sie sich mit Ihrem neuen Kennwort anmelden kŒnnen.\n\n\
   Bei weiteren Fragen wenden Sie sich bitte an Ihren Systemverwalter.    
12 12 : CUE kann /bin/passwd nicht ausfœhren.\n\
     Wenden Sie sich bitte an Ihren Systemverwalter.
13 13 : CUE konnte "callbacks" nicht binden. Prœfen Sie, ob CUE korrekt\n\
     installiert ist. Versuchen Sie es anschlieﬁend erneut.
14 14 : CUE kann /bin/cue.etc/cuesession nicht ausfœhren.\n\
     Wenden Sie sich bitte an Ihren Systemverwalter.
15 15 : CUE-Anmeldung fehlgeschlagen. Falsche Eingabe fœr Benutzername\n\
     oder Kennwort.
16 16 : CUE konnte Meldung Nr. %d aus Set %d nicht abrufen.\n\
     Prœfen Sie, ob Ihre Umgebungsvariablen LANG und NLSPATH\n\
     zum NLS-Meldungskatalog cue.cat passen.\n\
     NÃheres hierzu finden Sie in den Hilfeseiten environ(5) und lang(5).\n\
     Wenden Sie sich bitte an Ihren Systemverwalter.
17 17 : CUE steht nicht genœgend Hauptspeicher zur Verfœgung.\n\
     Wenden Sie sich bitte an Ihren Systemverwalter.
18 18 : CUE kann Ihre Shell %s nicht starten.\n\
     Wenden Sie sich bitte an Ihren Systemverwalter.
19 19 : CUE konnte die Bildschirmdatei /bin/cue.etc/cue.dm nicht laden.\n\
     Prœfen Sie, ob CUE korrekt installiert ist.\n
20 20 : CUE konnte den gewœschten Session-Typ im System nicht finden.\n\
     Bitte wÃhlen Sie einen anderen Session-Typ.
21 21 : CUE kann die 'uid' nicht zurœcksetzen. Stellen Sie sicher, daﬁ\n\
     das setuid-Bit fœr /bin/cue gesetzt ist. Wenden Sie sich bitte\n\
     an Ihren Systemverwalter.
22 22 : CUE kann /usr/lib/nls/config nicht Œffnen. Wenden Sie sich\n\
     bitte an Ihren Systemverwalter.
23 23 : EingeschrÃnkte Shell. CUE kann nicht von Befehlszeile starten.
