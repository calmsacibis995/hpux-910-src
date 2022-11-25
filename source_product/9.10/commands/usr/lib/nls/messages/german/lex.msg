
$
$ (c) Copyright 1990, OPEN SOFTWARE FOUNDATION, INC.
$ ALL RIGHTS RESERVED
$
$
$
$ RESTRICTED RIGHTS LEGEND
$ Use, Duplication or Disclosure by the Government is subject to
$ restrictions as set forth in paragraph (b)(3)(B) of the rights in
$ Technical Data and Computer Software clause in DAR 7-104.9(a).
$ 
$ @(#)lex.msg1.4  com/cmd/lang/lex,3.1,8943 9/7/89 18:36:27
$
$ COMPONENT_NAME: (CMDLANG) Language Utilities
$
$ FUNCTIONS: 
$
$ ORIGINS: 27
$
$ (C) COPYRIGHT International Business Machines Corp. 1988, 1989
$ All Rights Reserved
$ Licensed Materials - Property of IBM
$
$ US Government Users Restricted Rights - Use, duplication or
$ disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
$

$set 1		Commonly used words
$
$ These words appear in most messages printed to the user.  For example,
$ message 2 and 3 are used in the message
$   "File.l" line 0: warning 2: Unknown option 'u'
$
$quote "
$
1	"Fehler"
2	"Warnung"
3	"Zeile"
$
$ String 4 is used as the colon seperator.  It is used twice in the
$ above warning message example.
$
4	":"

$set 2	Message Text
$
$ This set consists of the error messages, warning messages and output
$ diagnostics given to the user.
$
$quote "
1	"Option !1 unbekannt\n"
2	"Ungœltiger Zustand !1 !2\n"
$
$ L10N note: Leave '%e' as it is.  It is a source construct.
$	     Lex has a number or source directives that allow users to specifiy
$	     sizes for internal data structures.  %e is one of them.  
$	     This message is telling the user to add a line to in the input 
$	     specification similar to "%e 1000".
$
3	"Parse-Baum zu groﬁ, %e <Zahl> verwenden\n"
4	"Parse-Baum zu groﬁ\n"
5	"Kein Hauptspeicher zuweisbar - calloc gibt 0 zurœck\n"
$
$ L10N note: calloc is a unix system routine that is called to allocate memory.
$	     I suggest you translate the message to the equivalent of 
$		"Unable to allocate memory - calloc returns a 0"
$
6	"Zu viele Zeichen in den Keller (stack) gestellt\n"
7	"Zeichenwert !1 auﬁerhalb des Bereichs\n"
8	"Zeichentabelle muﬁ neu definiert werden\n"
9	"Zeichen !1 zweimal verwendet\n"
10	"Zeichen '!1' zweimal verwendet\n"
$
$ L10N note: Leave '%p' as it is.  It is a source construct.
$	     See the comment of msg 3.
$
11	"Zu viele Stellen. %p <Zahl> verwenden\n"
12	"Zu viele Stellen\n"
13	"Zeichenfolge nicht abgeschlossen\n"
$
$ L10N note: Leave '%a' as it is.  It is a source construct.
$	     See the comment of msg 3.
$
14	"Zu viele €bergÃnge. %a <Zahl> verwenden\n"
15	"Zu viele €bergÃnge\n"
16	"EOF innerhalb eines Kommentars\n"
17	"EOF innerhalb Zeichenfolge oder Zeichenkonstante\n"
18	"Zu viele Stellen fœr einen Zustand - acompute\n"
$
$ L10N note: acompute is a subroutine name.
$
19	"Kein Speicherplatz fœr Umkehrung der Zeichensatztabelle\n"
20	"Zu viele gœltige Kontexte. Option -Xa verwenden\n"
21	"Zeichenklasse œberschreitet MaximalgrŒﬁe von !1 Byte\n"
22	"Ausfœhrbare Anweisungen mœssen gleich nach %% stehen\n"
23	"Ungœltige Anforderung !1\n"
24	"Angabe zur Sprache erfolgt zu spÃt\n"
25	"Zeile !1: !2\n"
26	"Definitionen zu lang\n"
27	"Startbedingungen zu lang, Option -XS verwenden\n"
28	"Zu viele umfangreiche Zeichenklassen, Option -Xc verwenden\n"
29	"Zu viele Definitionen, Option -Xd verwenden\n"
30	"Zu viele ZustÃnde\n"
$
$ L10N note: Leave '%n' as it is.  It is a source construct.
$	     See the comment of msg 3.
$
31	"%n <Zahl> verwenden\n"
32	"Iterationsbereich muﬁ positiv sein\n"
33	"Zu wenig freier Speicher im Betriebssystemkern fœr Start\n"
34	"Zu wenig freier Speicher im Betriebssystemkern fœr Zustandsgenerierung\n"
35	"Zu wenig freier Speicher im Betriebssystemkern fœr endgœltiges Packen\n"
36	"Zu wenig freier Speicher im Betriebssystemkern fœr Parse-Baum\n"
37	"Definition !1 nicht gefunden\n"
38	"Eingabedatei !1 kann nicht gelesen werden\n"
39	"Standardeingabe kann nicht gelesen werden\n"
40	"Ersatzzeichenfolgen dœrfen nicht mit Ziffern beginnen\n"
41	"Lex-Treiber fehlt, Datei !1\n"
42	"Aktion wird nicht beendet\n"
43	"!1 kann nicht geŒffnet werden\n"
44	"Datei !1 kann nicht geŒffnet werden\n"
45	"Zeichenklasse nicht portierbar\n"
46	"Ratfor wird von Lex momentan nicht unterstœtzt\n"
47	"Begrenzungszeichen fœr Regelabschnitt fehlt\n"
48	"Startbedingung !1 nicht definiert\n"
49	"Keine €bersetzung - Null-Zeichenfolge angenommen\n"
$
$ L10N note: Leave '%o' as it is.  It is a source construct.
$	     See the comment of msg 3.
$
50	"€berlauf der Ausgabetabelle, %o <Zahl> verwenden\n"
51	"EOF tritt zu frœh auf\n"
52	"Shift-Zeichen gefunden\n"
53	"Zeichenfolge zu lang\n"
$
$ L10N note: Leave '%k' as it is.  It is a source construct.
$
54	"Zu viele gepackte Zeichenklassen, %k <Zahl> verwenden\n"
55	"Zu viele Startbedingungen, Option -Xs verwenden\n"
56	"Zu viele Startbedingungen definiert, Option -Xs verwenden\n"
$ 57 is open for use.
58	"ZusÃtzlicher SchrÃgstrich entfernt\n"
$
$ Messages 59-62 end up concatenated on a single line.	They are output if
$ the user has requested a "summary" of the machine generated.	The -v 
$ option is used to generate the output.  All optional arguments are
$ filled with integer values.
$
59	"!1/!2 Knoten(%e), !3/!4 Stellen(%p), !5/!6 (%n), !7 €bergÃnge, "
60	"!1/!2 gepackte Zeichenklassen(%k), "
61	"!1/!2 gepackte €bergÃnge(%a), "
62	"!1/!2 Ausgabe-Slots(%o)\n"
63	"Ungœltiger Bereich [!1-!2]. 'From'-Wert > 'To'-Wert\n"
64	"Ungœltiger Bereich [!1!2-!3!4]. 'From'-Wert > 'To'-Wert\n"
65	"Elemente eines Bereiches mœssen zur gleichen Zeichenmenge gehŒren\n"
66	"Bereiche mœssen entweder nur durch Groﬁ- oder nur durch Kleinbuchstaben begrenzt werden\n"
67	"Ungœltiges Sortiersymbol [. .]\n"
68	"Ungœltige ÿquivalenzklasse ([= =])\n"
69	"Ungœltiger Ausdruck fœr Zeichenklasse ([: :])\n"
70	"Option -X ungœltig (ignoriert)\n"
71	"Angegebene TabellengrŒﬁe zu klein (ignoriert)\n"
72	"Tabellenbezeichner nicht erkannt -N!1 (ignoriert)\n"
73	"Ungœltiger €bergang !1 !2\n"
74	"Leeres Sortiersymbol gefunden ([. .]), ignoriert\n"
75	"LÃnderspezifische Angabe erfolgt zu spÃt\n"
$
$ L10N note: Leave '%l' as it is.  It is a source construct.
$ !1 is replaced with any invalid locale such as japanese or chinese-t.
$
76	"Lokale Angabe '!1' fœr %l nicht unterstœtzt, weiter mit Standardangabe\n"
77	"^-Operator in CCLs ([ ]) mit Multibyte-Zeichen nicht unterstœtzt\n"
78	"Multibyte-Zeichen kŒnnen mit [. .], [= =] bzw. [: :] nicht gemischt werden\n"
79	"Syntaxfehler - Sicherungslauf nicht mŒglich\n"
80	"Yacc-Kellerœberlauf\n"
81	"Syntaxfehler\n"
82	"Es kann kein Speicher fœr Yacc-Keller zugeordnet werden\n"
