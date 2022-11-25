
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
1	"ъщт╝"
2	"у╖и╥"
3	"▀▄"
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
1	"бй╗╙╠▀ш┤юш !1\n"
2	"╚╗╡ўъщт╝ !1 !2\n"
$
$ L10N note: Leave '%e' as it is.  It is a source construct.
$	     Lex has a number or source directives that allow users to specifiy
$	     sizes for internal data structures.  %e is one of them.
$	     This message is telling the user to add a line to in the input
$	     specification similar to "%e 1000".
$
3	"жг╜▓┐тненв, с№╩ї %e num\n"
4	"жг╜▓┐тненв\n"
5	"╞р┬║е┌щ╣с╒╢╞ё─ - calloc кў╢╬бвг├ 0\n"
$
$ L10N note: calloc is a unix system routine that is called to allocate memory.
$	     I suggest you translate the message to the equivalent of
$		"Unable to allocate memory - calloc returns a 0"
$
6	"║╨чшнем∙пбд╪\n"
7	"пбд╪г╥ !1 ф№чЎ╥┐ло\n"
8	"ch ▀ь┐·э╙рўщє╗зп╩и╥\n"
9	"пбд╪ !1 вь╩їчЎдь└└\n"
10	"пбд╪'!1' вь╩їчЎдь└└\n"
$
$ L10N note: Leave '%p' as it is.  It is a source construct.
$	     See the comment of msg 3.
$
11	"нем∙в╥╓▓, с№╩ї %p num\n"
12	"нем∙в╥╓▓\n"
13	"╞р╘о╘╡пбб║\n"
$
$ L10N note: Leave '%a' as it is.  It is a source construct.
$	     See the comment of msg 3.
$
14	"нем∙у╗║Ё, с№╩ї %a num\n"
15	"нем∙у╗║Ё\n"
16	"EOF в╥╗нсшс╝дъ\n"
17	"EOF в╥╗нпбб║╢Їпбд╪▓ж║щб╖\n"
18	"бв╨╥╚╗╡ў╝╘нем∙в╥╓▓ - acompute\n"
$
$ L10N note: acompute is a subroutine name.
$
19	"╞р╨єыЁилп╓╙ёзєи╜пбд╪▀ь\n"
20	"нем∙иогчбзби║Ё, т╠с№ -Xa ш┤юш\n"
21	"пбд╪э│ф№чЎ !1 в╥д╪╘░╠▀╝╧нв╛№ь╤\n"
22	"иллъ▀▄б╞║╒ч─╢╬╗н %% б╞│Ї╤╝з╥╩ё╩я\n"
23	" !1 рў┴╓бй╤┌\n"
24	"нешп╕лп╞т╕║Ё\n"
25	"▀▄▌ы !1: !2\n"
26	"п╞╓╤неыу\n"
27	"й═нт╛▒в╢неыу, т╠с№ -XS ш┤юш\n"
28	"нем∙нв╠▀пбд╪э│, т╠с№ -Xc ш┤юш\n"
29	"нем∙п╞╓╤, т╠с№ -Xd ш┤юш\n"
30	"нем∙╚╗╡ў\n"
$
$ L10N note: Leave '%n' as it is.  It is a source construct.
$	     See the comment of msg 3.
$
31	"с№╩ї %n num\n"
32	"быэ│╥┐ло┤░юы╟▀└▄г╥\n"
33	"└├ыынтб╞╜ч┤п┐·не░л\n"
34	"╚╗╡ў╩ё╩яб╞╜ч┤п┐·не░л\n"
35	"╝╧│Ї║ци╡б╞╜ч┤п┐·не░л\n"
36	"жг╜▓┐тб╞╜ч┤п┐·не░л\n"
37	"п╞╓╤ !1 ╖╩бйеь\n"
38	"╞р┬║у┴зїцтдщ┐· !1\n"
39	"╞р┬║у┴зї┐╘─ецтдщ\n"
40	"пбб║╝╬впбйилв▒║щпб╟▀ф∙нт\n"
41	"╒ї░м Lex Ё№ж╙╨─│▓, ┐·╜є !1\n"
42	"ж╙в▐░п╝ц╘о╘╡\n"
43	"╞р┬║ыый═ !1 \n"
44	"╞р┬║ыый═ !1 ┐·╜є\n"
45	"э¤ил║╝б╞пбд╪э│\n"
46	"lex ═╖е№╝ц║╟╣л Ratfor\n"
47	"╒ї░мсбе°шяв╣е┌ьЇпбд╪\n"
48	"╝цп╞╓╤б╞ф∙нт╛▒в╢ !1\n"
49	"╝цчш▀▄╓єу╗ - г▀п╞╟▀╨єпбб║\n"
$
$ L10N note: Leave '%o' as it is.  It is a source construct.
$	     See the comment of msg 3.
$
50	"цте╘▀ь┐·─ле╘, с№╩ї %o num\n"
51	"eof бйп┐║ц\n"
52	"╠┌╔╨пбд╪╨┐в╥.\n"
53	"пбб║неыу\n"
$
$ L10N note: Leave '%k' as it is.  It is a source construct.
$
54	"нем∙╘ы╪]пбд╪э│, с№╩ї %k num\n"
55	"вь╩їнем∙ф∙нт╛▒в╢, т╠с№ -Xs ш┤юш\n"
56	"п╞╓╤нем∙ф∙нт╛▒в╢, т╠с№ -Xs ш┤юш\n"
$ 57 is open for use.
58	"ехь▌м∙яЁ╠▀║∙╪D\n"
$
$ Messages 59-62 end up concatenated on a single line.	They are output if
$ the user has requested a "summary" of the machine generated.  The -v
$ option is used to generate the output.  All optional arguments are
$ filled with integer values.
$
59	"!1/!2 ╥╜Ї│(%e), !3/!4 в╥╓▓(%p), !5/!6 (%n), !7 у─ш│, "
60	"!1/!2 ╘ы╪]пбд╪э│(%k), "
61	"!1/!2 у─ш│╘ы╪](%a), "
62	"!1/!2 цте╘┐═(%o)\n"
63	"╥┐лобй░и [!1-!2]. 'ыынт' г╥ > '╘╡╝■' г╥\n"
64	"╥┐лобй░и [!1!2-!3!4]. 'ыынт' г╥ > '╘╡╝■' г╥\n"
65	"╥┐лоб╖╠▀рўв╢┤░юы░┘╗ни╕бвг├пбд╪э│\n"
66	"╥┐лоб╖╠▀дьш├пбд╪║щнв░л╢╬═║и╕\n"
67	"╜ф░и╤┌▌ыбй░и [. .]\n"
68	"░и╤шя╡ечбй░и ([= =])\n"
69	"пбд╪я╡еччЇ╥к│▓бй░и ([: :])\n"
70	"-X ш┤юшбй░и (╦╖чЎ)\n"
71	"╕лп╞б╞▀ь┐·не░л (╦╖чЎ)\n"
72	"╞р┬║чиу▓╕лп╞б╞▀ь┐· -N!1 (╦╖чЎ)\n"
73	"у─ш│ъщт╝ !1 !2\n"
74	"╠┌╔╨╨є╠▀╜ф░и╤┌▌ы ([. .]), ╦╖чЎ.\n"
75	"нешп╕лп╞═╖┐╘\n"
$
$ L10N note: Leave '%l' as it is.  It is a source construct.
$ !1 is replaced with any invalid locale such as japanese or chinese-t.
$
76	"бй║╟╣л═╖┐╘ '!1' еь %l, в▒дъп╞═╖┐╘чш▀▄\n"
77	"CCLs б╖бй║╟╣ли╔╝╘м∙в╥д╪╘░пбд╪╠▀ ^ чЇ╥кд╪([ ])\n"
78	"╞р┬║├╔и╡м∙в╥д╪╘░пбд╪ию [. .], [= =], ╢Ї [: :]\n"
79	"т╕┬║ъщт╝ - ╞р┬║гЇ╩ї\n"
80	"yacc ля╦╟─лв╥\n"
81	"т╕┬║ъщт╝\n"
82	"╞р┬║╟▀ yacc ля╦╟щ╣╓▓╨єыЁ\n"
