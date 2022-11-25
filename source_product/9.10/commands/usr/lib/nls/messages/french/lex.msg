
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

$set 1          Commonly used words
$
$ These words appear in most messages printed to the user.  For example,
$ message 2 and 3 are used in the message
$   "File.l" line 0: warning 2: Unknown option 'u'
$
$quote "
$
1	"erreur"
2	"attention"
3	"ligne"
$
$ String 4 is used as the colon seperator.  It is used twice in the
$ above warning message example.
$
4	":"

$set 2  Message Text
$
$ This set consists of the error messages, warning messages and output
$ diagnostics given to the user.
$
$quote "
1	"Option !1 inconnue\n"
2	"≈tat !1 !2 erron≈\n"
$
$ L10N note: Leave '%e' as it is.  It is a source construct.
$            Lex has a number or source directives that allow users to specifiy
$            sizes for internal data structures.  %e is one of them.
$            This message is telling the user to add a line to in the input
$            specification similar to "%e 1000".
$
3	"Arborescence d'analyse syntaxique trop volumineuse, essayez d'utiliser %e n\n"
4	"Arborescence d'analyse syntaxique trop volumineuse\n"
5	"Impossible d'allouer de la m≈moire, calloc a retourn≈ un 0\n"
$
$ L10N note: calloc is a unix system routine that is called to allocate memory.
$            I suggest you translate the message to the equivalent of
$               "Unable to allocate memory - calloc returns a 0"
$
6	"Trop de caract…res introduits\n"
7	"La valeur du caract…re !1 est hors s≈quence\n"
8	"la table ch n≈cessite une nouvelle d≈claration\n"
9	"Caract…re !1 utilis≈ deux fois\n"
10	"Caract…re '!1' utilis≈ deux fois\n"
$
$ L10N note: Leave '%p' as it is.  It is a source construct.
$            See the comment of msg 3.
$
11	"Trop de positions, Essayez d'utiliser %p \n"
12	"Trop de positions\n"
13	"Cha—ne non termin≈e\n"
$
$ L10N note: Leave '%a' as it is.  It is a source construct.
$            See the comment of msg 3.
$
14	"Trop de transitions, Essayez d'utiliser %a \n"
15	"Trop de transitions\n"
16	"Indicateur de fin de fichier dans le commentaire\n"
17	"Indicateur de fin de fichier dans la cha—ne ou constante de caract…res\n"
18	"Trop de positions pour un ≈tat - acompute\n"
$
$ L10N note: acompute is a subroutine name.
$
19	"Espace insuffisant pour l'inversion de la table de caract…res\n"
20	"Trop de contextes, essayez l'option -Xa\n"
21	"La classe de caract…re exc…de la taille maximale de !1 octets\n"
22	"Les instructions ex≈cutables doivent figurer apr…s %%\n"
23	"Demande !1 non valide\n"
24	"Trop tard pour sp≈cification de langue\n"
25	"ligne !1: !2\n"
26	"D≈finitions trop longues\n"
27	"Conditions START trop longues, essayez l'option -XS\n"
28	"Trop de grandes classes de caract…res, essayez l'option -Xc\n"
29	"Trop de d≈finitions, essayez l'option -Xd\n"
30	"Trop d'≈tats\n"
$
$ L10N note: Leave '%n' as it is.  It is a source construct.
$            See the comment of msg 3.
$
31	"Essayez d'utiliser %n\n"
32	"Plages de l'it≈ration doivent ¡tre positives\n"
33	"Noyau trop petit pour commencer\n"
34	"Noyau trop petit pour la g≈n≈ration d'≈tats\n"
35	"Noyau trop petit pour l'assemblage final\n"
36	"Noyau trop petit pour l'arborescence d'analyse syntaxique\n"
37	"D≈finition !1 introuvable\n"
38	"Impossible de lire le fichier d'entr≈e !1\n"
39	"Impossible de lire l'entr≈e standard\n"
40	"Les cha—nes de substitution ne commencent peut-¡tre pas par des chiffres\n"
41	"Pilote lex manquant, fichier !1\n"
42	"L'action ne se termine pas\n"
43	"Impossible d'ouvrir !1 \n"
44	"Impossible d'ouvrir le fichier !1\n" 
45	"Classe de caract…res non portable\n"
46	"Ratfor non accept≈ actuellement avec lex\n"
47	"D≈limiteur de section des r…gles manquant\n"
48	"Condition START !1 non d≈finie \n"
49	"Aucune traduction donn≈e - cha—ne nulle pr≈sum≈e\n"
$
$ L10N note: Leave '%o' as it is.  It is a source construct.
$            See the comment of msg 3.
$
50	"d≈bordement de la table des r≈sultats, essayez d'utiliser %o\n"
51	"Indicateur de fin de fichier pr≈matur≈\n"
52	"Caract…re majuscule rep≈r≈.\n"
53	"Cha—ne trop longue\n"
$
$ L10N note: Leave '%k' as it is.  It is a source construct.
$
54	"Trop de classes de caract…res condens≈s, essayez d'utiliser %k\n"
55	"Trop de conditions START utilis≈es, essayez l'option -Xs\n"
56	"Trop de conditions START d≈finies, essayez l'option -Xs\n"
$ 57 is open for use.
58	"Barre oblique en trop supprim≈e\n"
$
$ Messages 59-62 end up concatenated on a single line.  They are output if
$ the user has requested a "summary" of the machine generated.  The -v
$ option is used to generate the output.  All optional arguments are
$ filled with integer values.
$
59	"!1/!2 noeuds(%e), !3/!4 positions(%p), !5/!6 (%n), !7 transitions, "
60	"!1/!2 classes de caract…res condens≈s (%k), "
61	"!1/!2 transitions(%a) condens≈es, "
62	"!1/!2 intervalles(%o) de sortie\n"
63	"Plage non valide [!1-!2]. Valeur 'From' > valeur 'To'\n"
64	"Plage non valide [!1!2-!3!4]. Valeur 'From' > valeur 'To'\n"
65	"Les ≈l≈ments d'une plage doivent appartenir au m¡me jeu de caract…res\n"
66	"Les plages doivent avoir des caract…res de m¡me taille » chaque extr≈mit≈\n"
67	"Symbole d'interclassement non valide [. .]\n"
68	"Classe d'≈quivalence non valide ([= =])\n"
69	"Expression de classe de caract…res non valide ([: :])\n"
70	"Option -X non valide (non prise en compte)\n"
71	"Taille de la table sp≈cifi≈e trop petite (non prise en compte)\n"
72	"descripteur de table non reconnu -N!1 (non pris en compte)\n"
73	"transition !1 !2 erron≈e\n"
74	"Symbole d'interclassement vide rep≈r≈ ([. .]), non pris en compte.\n"
75	"Trop tard pour le descripteur de langue nationale\n"
$
$ L10N note: Leave '%l' as it is.  It is a source construct.
$ !1 is replaced with any invalid locale such as japanese or chinese-t.
$
76	"Langue nationale '!1' non prise en charge vers %l,\nvaleur par d≈faut utilis≈e\n"
77	"op≈rateur ^ non pris en charge dans les CCL ([ ]) contenant\ndes caract…res multi-octets\n"
78	"Impossible combiner caract…res multi-octets avec [. .], [= =] ou [: :]\n"
79	"erreur de syntaxe - impossible de sauvegarder\n"
80	"d≈bordement de la pile yacc\n"
81	"erreur de syntaxe\n"
82	"impossible d'allouer de l'espace pour les piles yacc\n"
