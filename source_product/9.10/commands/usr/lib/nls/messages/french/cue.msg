$ @(#) $Header: cue.msg,v 70.1 92/06/11 19:38:41 ssa Exp $
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
1 Fen¡tre d'ouverture de session
2 Hewlett-Packard
3 CUE (Environnement utilisateur de terminaux en mode caract…re)
4 Code utilisateur
5 Mot de passe
$
$ Do not translate the & character in any of the following messages.
$ Dialog Manager uses this notation to designate the character that
$ usres enter to signal their item selection from a menu, instead of using
$ the arrow keys.  For example the next entry (msg #6) instructs DM to
$ enable the user enter character C to select Configuration item from the
$ main menu instead of using the arrow keys to highlite their selection.
$
6 &Configuration
7 S≈lection de la &langue de la session
8 S≈lection du &type de session » l'ouverture
9 &Aide
10 &Introduction » CUE
11 Comment modifier la &langue de la session
12 Comment modifier le &type de session
13 &Lancer Session
14 &Fermer Session
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
15  Fermer Session
16 BRREMENUoui/non
17   Menu   Langue
18 &Ok
19 &Effacer
20 Menu Session
21 tsm   - Gestionnaire de session de terminal
22 keysh - Shell Menu tributaire du contexte
23 sh    - Shell de Bourne
24 csh   - C-shell
25 ksh   - Shell de Korn
26 Fen¡tre des messages d'erreur
27 Fen¡tre des messages d'aide
28 Effacer &zones
$
$ The following two messages (29 and 30) are FUNCTION KEY LABELS.
$
$  1234567812345678
29  Lancer session
30 Effacer  zones
31 S≈lectionnez la langue d'affichage.
$
$ The following two messages (32 and 33) are displayed together at once;
$ in other words, they should be treated as one message. The translation
$ should fit in two lines.
$
32 Positionnez le curseur avec les touches fl≈ch≈es et faites une
33 s≈lection avec Retour. Appuyez ensuite sur f8 (Ok).
34 S≈lectionnez le type de session de travail que vous d≈sirez.
$
$ The following two messages (35 and 36) are FUNCTION KEY LABELS.
$
35 Ok
36 Effacer
$
$ In the following message (37), the "<==" character sequence is a simple
$ pointing mechanism. It can be omitted from a translation, if it would make
$ it too complicated to understand. Also, a local version of a pointing
$ mechanism can be used instead, as long as the width of the message does
$ not exceed the 75-column limit.
$
37 <== Les donn≈es entr≈es dans cette zone ne seront pas affich≈es
$
$ Exclamation marks (!) are optional in the following message (38).
$ They are used for emphasis. They can be omitted or translated to a
$ suitable local character for that purpose.
$
38 N'oubliez pas de fermer la session !!!
$
$ The following message (39) can be completely translated, including the
$ colon (:) character.
$
39 Bienvenue :
40 Appuyez sur f4 (BRREMENU) pour s≈lectionner les options
41 Pour visualiser les donn≈es hors-≈cran:touches fl≈ch≈es ou SHIFT et fl…che
42 Apr…s avoir termin≈, appuyez sur f8 (Ok).
$ The following two messages (43 and 44) are FUNCTION KEY LABELS.
43   Aide
44  S≈lect

$set2 Messages from the script, cuesession
$ Please leave the punctuation as it is.
1001 Veuillez patienter jusqu'» ce que la session soit lanc≈e -
1002 Impossible de lancer le type de session s≈lectionn≈ -
1003 Avertissement CUE: Le r≈pertoire initial est "/".
1004 Pour quitter, entrez "exit" ou appuyez simultan≈ment sur Ctrl et d.
1005 Pour sortir de TSM, appuyez sur (f8) pour passer au menu Fin.

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
1 Nom  : OK\n\
But  : Permettre » l'utilisateur de revenir » la fen¡tre pr≈c≈dente\n\
       apr…s avoir effectu≈ les s≈lections d≈sir≈es\n\
Appel: Appuyez sur Retour ou sur f3 (S‹LECT)\n\
Saut : Utilisez TAB ou les touches fl≈ch≈es pour passer » une autre zone.

2 Nom  : Effacer\n\
But  : Effacer les s≈lections en cours et restaurer celles qui ≈taient en\n\
       vigueur » l'affichage du menu\n\
Appel: Appuyez sur Retour ou f3 (S‹LECT)\n\
Saut : Utilisez TAB ou les touches fl≈ch≈es pour passer » une autre zone.

3 Nom  : Code utilisateur\n\
But  : Zone d'entr≈e du code utilisateur\n\
Saut : Utilisez TAB ou les touches fl≈ch≈es pour passer » une autre zone\n\
       ou appuyez sur BRREMENU pour passer » la barre de menu\n\
Pr≈cisions:\n\
Entrez votre code utilisateur. Vous avez droit » 8 caract…res.\n\
Si vous faites une faute de frappe, vous pouvez la corriger en vous servant\n\
de la touche retour arri…re/suppr ou en s≈lectionnant le bouton-poussoir\n\
Effacer Zones; les donn≈es entr≈es seront supprim≈es.\n\n\
Apr…s avoir termin≈, appuyez sur Retour ou sur TAB\n\
ou sur une touche fl≈ch≈e pour passer » la zone suivante ou sur\n\
f4 (BRREMENU) pour passer » la barre de menu.

4 Nom   : Mot de passe\n\
But   : Zone d'entr≈e du mot de passe\n\
Action: Appuyez sur Retour apr…s avoir saisi le mot de passe, pour lancer\n\
        la session.\n\
Saut  : Utilisez TAB ou les touches fl≈ch≈es pour passer » une autre zone\n\
        ou appuyez sur BRREMENU pour passer » la barre de menu\n\
Pr≈cisions:\n\
Entrez le mot de passe. Pour des raisons de s≈curit≈,\n\
les caract…res entr≈s n'appara—tront pas » l'≈cran.\n\
Si vous faites une faute de frappe, vous pouvez la corriger en vous servant\n\
de la touche retour arri…re/suppr ou en s≈lectionnant le bouton-poussoir\n\
Effacer Zones pour effacer le contenu des zones Code utilisateur et\n\
Mot de passe.\n\n\
Appuyez ensuite sur TAB ou sur une touche fl≈ch≈e pour\n\
passer » la zone suivante, appuyez sur Retour pour lancer la session ou\n\
sur f4 (BRREMENU) pour passer » la barre de menu.

5 Nom  : Lancer Session\n\
But  : Validation par CUE des entr≈es du code utilisateur et du mot de\n\
       passe entr≈s avant le lancement de la session.\n\
Appel: Appuyez sur Retour ou sur f3 (S‹LECT)\n\
Saut : Utilisez TAB ou les touches fl≈ch≈es pour passer » une autre zone\n\
       ou appuyez sur BRREMENU pour passer » la barre de menu\n\
Pr≈cisions:\n\
Apr…s avoir entr≈ le code utilisateur et le mot de passe\n\
et s≈lectionn≈ des options dans la barre de menu,\n\
vous ¡tes pr¡t » lancer la session. Pour s≈lectionner\n\
ce bouton-poussoir, appuyez sur Retour ou sur\n\
f3 (S‹LECT). ° cette ≈tape, le code utilisateur\n\
et le mot de passe sont valid≈s. S'ils sont accept≈s\n\
la session est lanc≈e. S'ils sont refus≈s,\n\
essayez de les entrer de nouveau.\n\
Pour revenir » la zone Code utilisateur, servez-vous\n\
de la touche TAB ou des touches fl≈ch≈es, ou appuyez sur f4\n\
(BRREMENU) pour passer » la barre de menu.

6 Nom  : Effacer Zones\n\
But  : Effacer les donn≈es dans les zones Code utilisateur et Mot de passe.\n\
Appel: Appuyez sur Retour ou sur f3 (S‹LECT)\n\
Saut : Utilisez TAB ou les touches fl≈ch≈es pour passer » une autre zone\n\
       ou appuyez sur BRREMENU pour passer » la barre de menu.

7 Nom  : Fermer Session\n\
But  : Mettre fin au processus d'ouverture de session de CUE/\n\
       fermer la session\n\
Appel: Appuyez sur Retour ou sur f3 (S‹LECT)\n\
Saut : Utilisez TAB ou les touches fl≈ch≈es pour passer » une autre zone\n\
       ou appuyez sur BRREMENU pour passer » la barre de menu.

8 Nom   : Menu Langue\n\
But   : Changer la langue de ces ≈crans et celle de la session\n\
        de travail.\n\
Appel : Appuyez sur Retour ou sur f3 (S‹LECT)\n\
Saut  : Utilisez les touches fl≈ch≈es pour passer » une autre option\n\
        de la barre de menu ou appuyez de nouveau sur BRREMENU pour\n\
        sortir de la barre.\n\
D≈faut: La langue par d≈faut, C, est choisie si vous\n\
        n'avez pas s≈lectionn≈ de langue dans CUE.\n\
Pr≈cisions: La variable environnementale $LANG est modifi≈e\n\
        en fonction de la s≈lection.

9 Nom   : Menu Type de session\n\
But   : Modifier le type de session que vous utilisez.\n\
Appel : Appuyez sur Retour ou sur f3 (S‹LECT)\n\
Saut  : Utilisez les touches fl≈ch≈es pour passer » une autre option\n\
        de la barre de menu ou appuyez sur BRREMENU de nouveau pour\n\
        sortir de la barre.\n\
D≈faut: Par d≈faut, le shell de Bourne - /bin/sh, sera s≈lectionn≈\n\
        si vous n'en avez pas encore choisi un type de session dans CUE.

10 Nom  : Initiation » CUE\n\
But  : Afficher l'aide en ligne sur l'utilisation g≈n≈rale de CUE\n\
       (comment lancer une session de travail ou se d≈placer dans les\n\
       menus, par exemple)\n\
Appel: Appuyez sur Retour ou sur f3 (S‹LECT)\n\
Saut : Utilisez les touches fl≈ch≈es pour passer » une autre option\n\
       de la barre de menu ou appuyez sur BRREMENU de nouveau pour\n\
       sortir de la barre.

11 Nom       : S≈lecteur de langue\n\
But       :  Changer la langue d'affichage des menus et celle du texte.\n\
S≈lection : Utilisez les touches fl≈ch≈es, SHIFT et fl…che (page haut/bas),\n\
            et Page pr≈c/suiv (d≈filement horizontal) pour positionner\n\
            le curseur sur la langue de votre choix.\n\
            Puis, appuyez sur Retour ou sur f3 (S‹LECT) pour choisir\n\
            une option\n\
D≈faut    : La langue par d≈faut, C, est choisie si vous\n\
            n'en avez pas s≈lectionn≈ dans CUE.\n\
Pr≈cisions: La variable environnementale $LANG est modifi≈e\n\
            en fonction de la s≈lection.\n\
Retour    : Appuyez sur f8 (Ok) ou utilisez TAB pour positionner\n\
            le curseur sur Ok, puis appuyez sur Retour pour valider\n\
            le changement de langue et passer » la fen¡tre pr≈c≈dente.

$
$ Maintain formating by observing the number of space characters
$ in the Details portion of message 12.
$
12 Nom       : S≈lecteur de session\n\
But       : Modifier le type de session que vous utilisez.\n\
S≈lection : Utilisez les touches fl≈ch≈es, SHIFT et fl…che (page haut/bas),\n\
            et Page pr≈c/suiv (d≈filement horizontal) pour positionner\n\
            le curseur sur la langue de votre choix.\n\
            Puis, appuyez sur Retour ou f3 (S‹LECT) pour choisir\n\
            une option\n\
Retour    : Appuyez sur f8 (Ok) ou utilisez TAB pour positionner\n\
            le curseur sur Ok, puis appuyez sur Retour pour valider\n\
            le changement de langue et passer » la fen¡tre pr≈c≈dente.\n\
Pr≈cisions: Une fois la session ouverte, vous pouvez obtenir des pr≈cisions\n\
sur le type de session s≈lectionn≈ en entrant : man <type de session>\n\
Ainsi, pour obtenir des pr≈cisions sur sh, entrez:\n\
            man sh\n\n\
Type de session Description\n\
_______________ ________________________________________________________\n\
sh              Le shell de Bourne est le langage de programmation\n\
                standard ou "l'interpr≈teur" qui ex≈cute les commandes\n\
                d'un terminal ou d'un fichier. Il est utilis≈\n\
                comme type de session par d≈faut.\n\n\
ksh             Le shell de Korn (ksh) est une extension de sh comprenant\n\
                plusieurs fonctions suppl≈mentaires, comme la modification\n\
                de l'historique des commandes, l'abr≈viation et la\n\
                localisation des commandes et les expressions de condition.\n\n\
keysh           Le shell Menu est une extension du shell de Korn standard.\n\
                Il utilise une structure hi≈rarchique de menus de\n\
                touches programmables, offre de l'aide contextuelle aux\n\
                utilisateurs pour la constitution de lignes de commandes\n\
                et combine la puissance du shell de Korn avec la\n\
                convivialit≈ d'un menu syst…me. Il est enti…rement articul≈\n\
                autour des donn≈es, ce qui permet d'augmenter facilement ses\n\
                menus et l'aide.  Lors de l'apprentissage d'HP-UX, nous\n\
                conseillons d'utiliser le shell Menu.\n\n\
csh             C-shell (csh) est un interpr≈teur de commandes\n\
                qui int…gre un tampon de l'historique des\n\
                commandes, une syntaxe similaire » celle de C et des\n\
                fonctions de contr¬le des travaux.\n\n\
tsm             Le gestionnaire de sessions de terminal (TSM)\n\
                est syst…me multifen¡tre pour les terminaux ASCII.\n\
                Il offre un acc…s rapide » 10 sessions, semblable »\n\
                des couches de shell, shl(1), tout en conservant le\n\
                contexte visuel et la pr≈sentation de chaque session.\n\
                Toutes les sessions sous TSM ont acc…s » toutes les\n\
                fonctions du terminal et emplissent compl…tement l'≈cran.\n\
                C'est un outil inestimable, lorsque vous connaissez\n\
                bien HP-UX.

13 Nom      : S≈lecteur des messages d'aide\n\
But      : Visualiser les messages d'aide\n\
Affichage: Utilisez les touches fl≈ch≈es, SHIFT et fl…che (page haut/bas),\n\
           et Page pr≈c/suiv (d≈filement horizontal) pour visualiser\n\
           les messages d'erreur hors-≈cran.\n\
Retour   : Appuyez sur f8 (Ok) ou utilisez TAB pour positionner\n\
           le curseur sur bouton-poussoir Ok, puis appuyez sur Retour\n\
           pour valider le changement de langue et passer » la\n\
           fen¡tre pr≈c≈dente.

14 Nom      : S≈lecteur des messages d'erreur\n\
But      : Visualiser les messages d'erreur\n\
Affichage: Utilisez les touches fl≈ch≈es, SHIFT et fl…che (page haut/bas),\n\
           et Page pr≈c/suiv (d≈filement horizontal) pour visualiser\n\
           les messages d'erreur hors-≈cran.\n\
Retour   : Appuyez sur f8 (OK) ou utilisez TAB pour positionner le curseur\n\
           sur le bouton-poussoir Ok, puis appuyez sur Retour pour valider\n\
           le changement de langue et revenir » la fen¡tre pr≈c≈dente.

$
$ Maintain format by preserving the extra spaces so that the message
$ is lined up in the Screen Navigation portion:
$
15 Nom      : Initiation » CUE\n\
But      : Vous initier » CUE\n\
Affichage: Utilisez les touches fl≈ch≈es, SHIFT et fl…che (page haut/bas),\n\
           et Page pr≈c/suiv (d≈filement horizontal) pour visualiser\n\
           les messages d'aide hors-≈cran.\n\
Pr≈cisions:\n\
           La fonction principale de CUE est de vous permettre\n\
           d'ouvrir une session dans votre syst…me et de commencer\n\
           » personnaliser votre session de travail. Il comprend des ≈crans\n\
           attrayants des menus utiles et des fen¡tres d'aide conµus pour\n\
           vous venir en aide dans votre session de travail.\n\
Affichage de l'aide:\n\
           Appuyez sur (f1) AIDE, chaque fois que vous d≈sirez des\n\
           informations sur une zone, un bouton-poussoir ou une option\n\
           de menu.\n\
Personnalisation de votre session de travail:\n\
           Avant d'entrer le code utilisateur et le mot de passe, vous\n\
           devez configurer dans la session CUE :\n\n\
              o la langue d'affichage\n\
              o le type de session » utiliser comme session \n\
                de travail » l'ouverture\n\
           Appuyez sur (f4) BRREMENU et s≈lectionnez Configuration\n\
           pour personnaliser ces param…tres. Pour sortir de la barre de\n\
           menu, appuyez de nouveau sur BRREMENU.\n\
Lancement de la session:\n\
           Une fois la session de travail personnalis≈e, vous pouvez\n\
           entrer votre code utilisateur et votre mot de passe. Lorsque\n\
           vous appuyez sur Retour dans la zone Mot de passe, le code\n\
           utilisateur et le mot de passe sont compar≈s avec ceux du\n\
           syst…me. S'ils sont valid≈s, la session de travail est lanc≈e.\n\
           Dans le cas contraire, vous devez les entrer de nouveau.\n\
D≈placement » l'≈cran:\n\
           Cette section d≈crit comment se d≈placer dans les\n\
           ≈crans CUE.\n\n\
   Touches:\n\
      fl…che vers le haut/bas      - passer d'une ligne » l'autre\n\
      fl…che vers la gauche/droite - passer d'une colonne » l'autre\n\
      Tab                          - passer » la zone suivante\n\
      Shift et fl…che haut/bas     - passer » la page suivante/pr≈c≈dente\n\
      BARRE D'ESPACEMENT           - s≈lectionner\n\
      Retour                       - valider une entr≈e ou une s≈lection\n\
      Retour arri…re               - effacer un caract…re\n\
      Page suivante                - d≈filer horizontalement d'une page\n\
                                     vers la droite\n\
      Page pr≈c                    - d≈filer horizontalement d'une page\n\
                                     vers la gauche\n\
      Effacer ‹cran                - r≈g≈n≈rer l'≈cran\n\
   Touches de fonction g≈n≈rales (f1-f8):\n\
      AIDE (F1)                    - Obtenir de l'aide sur l'≈l≈ment sous\n\
                                     le curseur.\n\
      S‹LECT (F3)                  - S≈lectionner un bouton-poussoir, une\n\
                                     option ou un ≈l≈ment dans une liste.\n\
      BRREMENU (F4)                - S≈lectionner une option de la barre.\n\
   Touches de fonction de la fen¡tre d'ouverture de session (f1-f8):\n\
      Lancer Session (F5)          - Valider le code utilisateur et le mot\n\
                                     de passe, ouvrir la session de travail.\n\
      Effacer Zones (F6)           - Effacer les donn≈es entr≈es dans les\n\
                                     zones Code utilisateur et Mot de passe.\n\
      Fermer Session (F8)          - Mettre fin » la session CUE ou fermer\n\
                                     la session.\n\
   Touches de fonction de la fen¡tre langue/session (f1-f8):\n\
      Effacer (F5)                 - Effacer l'entr≈e en cours et restaurer\n\
                                     l'entr≈e s≈lectionn≈e » l'affichage\n\
                                     du menu.\n\
      Ok (F8)                      - Valider la s≈lection et passer » la\n\
                                     fen¡tre d'ouverture de session.\n\
   Touches de fonction de la fen¡tre aide/erreur (f1-f8):\n\
      Ok (F8)                      - Appuyez sur cette touche apr…s avoir\n\
                                     lu les messages.

16 Nom       : Comment modifier la langue de la session\n\
   But    : Modifier la langue de vos ≈crans\n\
Pr≈cisions: S≈lectionnez Configuration dans la barre de menu\n\
            en entrant un retour. Un autre menu s'affiche, dans lequel\n\
            vous pouvez s≈lectionner l'option Menu Langue.\n\
Retour    : Pour sortir de la barre de menu, appuyez de nouveau sur (f4)\n\
            BRREMENU.

17 Nom       : Comment modifier le type de session\n\
Pr≈cisions: S≈lectionnez Configuration dans la barre de menu\n\
            en appuyant sur Retour. Un autre menu s'affiche, dans lequel\n\
            vous pouvez s≈lectionner l'option Menu Type de session.\n\
Retour    : Pour sortir de la barre de menu, appuyez sur f4 (BRREMENU).

18 Nom  : Comment modifier la langue de la session\n\
But  : Afficher l'aide en ligne sur la faµon de modifier\n\
       la langue d'affichage de CUE et celle de la\n\
       session de travail.\n\
Appel: Appuyez sur Retour ou appuyez sur f3 (S‹LECT)\n\
Saut : Utilisez les touches fl≈ch≈es pour passer » une autre option de\n\
       la barre de menu ou appuyez sur BRREMENU pour sortir de la barre.

19 Nom  : Comment modifier le type de session\n\
But  : Afficher l'aide en ligne sur la faµon de modifier\n\
       le type de session de travail » l'ouverture, une fois\n\
       le code utilisateur et le mot de passe valid≈s.\n\
Appel: Appuyez sur retour ou sur f3 (S‹LECT)\n\
Saut : Utilisez les touches fl≈ch≈es pour passer » une autre option de\n\
       la barre de menu ou appuyez sur BRREMENU pour sortir de la barre.

$set4 NLS Language table must correlate with the nls_lang array in cue.c
$ DO NOT localize the NLS languages in parenthesis ()'s. Also, do NOT
$ change the order of entries.
$
1 Correspondant » D‹FAUT (C) avec le symbole mon≈taire $ (n-computer)
2 Am≈ricain (american)
3 Canadien Franµais (c-french)
4 Danois
5 Hollandais
6 Anglais (Grande-Bretagne) (english)
7 Finnois
8 Franµais
9 Allemand
10 Italien
11 Norv≈gien
12 Portugais
13 Espagnol
14 Su≈dois
15 Islandais
16 Japonais avec l'alphabet phon≈tique Katakana (katakana)
17 Arabe
18 Arabe d'Afrique du Nord sans les chiffres Hindi (arabic-w)
19 Grec
20 H≈breu
21 Turc
22 Tha›
23 Langue par D‹FAUT recommand≈e par HP, d≈finie par X/OPEN (C)
24 POSIX
25 Anglais am≈ricain pour les terminaux ISO 8859/1 (american.iso88591)
26 Franµais canadien  pour les terminaux ISO 8859/1 (c-french.iso88591)
27 Danois pour les terminaux ISO 8859/1 (danish.iso88591)
28 Hollandais pour les terminaux ISO 8859/1 (dutch.iso88591)
29 Anglais (G-Bretagne) pour les terminaux ISO 8859/1 (english.iso88591)
30 Finnois pour les terminaux ISO 8859/1 (finnish.iso88591)
31 Franµais pour les terminaux ISO 8859/1 (french.iso88591)
32 Allemand pour les terminaux ISO 8859/1 (german.iso88591)
33 Italien pour les terminaux ISO 8859/1 (italian.iso88591)
34 Norv≈gien pour les terminaux ISO 8859/1 (norwegian.iso88591)
35 Portugais pour les terminaux ISO 8859/1 (portuguese.iso88591)
36 Espagnol pour les terminaux ISO 8859/1 (spanish.iso88591)
37 Su≈dois pour les terminaux ISO 8859/1  (swedish.iso88591)
38 Islandais pour les terminaux ISO 8859/1 (icelandic.iso88591)
39 Tch…que
40 Hongrois
41 Polonais
42 Roumain
43 Serbo-croate
44 Slov…ne
45 Russe
46 Bulgare
47 Chinois simplifi≈ utilis≈ en Chine continentale (chinese-s)
48 Chinois traditionnel utilis≈ » Taiwan (chinese-t)
49 Chinois traditionnel avec le jeu de caract…res Big 5 (chinese-t.big5)
50 Japonais, caract…res Kanji et le codage HP-15 (japanese)
51 Japonais, caract…res Kanji et le codage EUC (japanese.euc)
52 Cor≈en
53 Arabe avec le jeu de caract…res ISO 88596 (arabic.iso88596)
54 Grec avec le jeu de caract…res ISO 88597 (greek.iso88597)
55 H≈breu avec le jeu de caract…res ISO 88598 (hebrew.iso88598)
56 Turc avec le jeu de caract…res ISO 88599 (turkish.iso88599)

$set5 Error Messages
$
$ Strings of the form %<char> where <char> is a single character should
$ not be localized.
$ When multiple lines are present for a message, indent all lines after the
$ first line, so that all lines line up with the left edge of the first line.
$ Other extra spaces are also present for formating and therefore should be
$ preserved.
$
1 1 : Avertissement CUE. Sous-syst…me racine : %s.
2 2 : CUE n'a pas pu ouvrir le catalogue de messages de la\n\
    langue choisie. CUE continuera » utiliser la langue en\n\
    cours, mais la session de travail sera dans la langue\n\
    que vous venez de choisir.
3 3 : Avertissement CUE. R≈pertoire initial : /.
4 4 : CUE est incapable de passer au r≈pertoire INITIAL, %s.\n\
    Assurez-vous qu'une entr≈e du fichier mot de passe\n\
    associ≈e » votre r≈pertoire INITIAL existe et que ce dernier\n\
    est accessible. Adressez-vous » l'administrateur du syst…me.
5 5 : CUE : le nombre maximal de sessions ouvertes est atteint.\n\
    R≈essayez plus tard.
6 6 : CUE : Aucune entr≈e /etc/utmp. Essayez d'utiliser : exec cue.
7 7 : CUE : Aucun r≈pertoire racine pour la r≈ex≈cution du programme login.\n\
    Adressez-vous » l'administrateur du syst…me.
8 8 : CUE : /bin/cue introuvable dans le r≈pertoire racine (/).\n\
    Adressez-vous » l'administrateur du syst…me.
9 9 : CUE : Le num≈ro d'identification du groupe dans le fichier\n\
    mot de passe est erron≈. Adressez-vous » l'administrateur du syst…me.
10 10 :CUE: Le num≈ro d'identification de l'utilisateur dans le fichier\n\
    mot de passe est erron≈. Adressez-vous » l'administrateur du syst…me.
$
$ Preserve additional spaces for proper formating and line-up of the
$ message text.
$
11 11 : CUE : Vous devez d≈finir un mot de passe; vous n'en\n\
avez pas ou il est expir≈.\n\n\
R…gles » suivre lors de la s≈lection d'un mot de passe :\n\
   * Il doit comprendre au moins six caract…res.\n\
     Seul les huit premiers caract…res sont significatifs.\n\
   * Il doit ¡tre diff≈rent du code utilisateur et de\n\
     toutes les combinaisons s'y rapportant (les majuscules\n\
     sont trait≈s comme les minuscules).\n\
   * Il doit comporter au moins trois caract…res diff≈rents par rapport\n\
     » l'ancien mot de passe (majuscules et minuscules).\n\
   * Il doit comporter au moins deux caract…res alphab≈tiques (majuscules\n\
     ou minuscules)\n\
   * Il doit comprendre au moins un caract…re :\n\
      - Num≈rique (0 » 9)\n\
      - Sp≈cial (ni alphab≈tique ni num≈rique).\n\
        Exemple : -, _ ou $.\n\n\
   Selon les r…gles ≈nonc≈es, les mots de passe suivants sont valides:\n\n\
      foo-bar   $argent$   Nombre_9   @bague\n\n\
   La distinction est faite entre les majuscules et les minuscules.\n\
   Ces mots de passe sont donc tous diff≈rents :\n\n\
      foo-bar   Foo-Bar   FOO-BAR\n\n\
   Essayez de choisir un mot de passe, que personne ne pourra\n\
   deviner. Si une personne connaissait votre mot de passe, elle\n\
   pourrait ouvrir une session et acc≈der » vos fichiers.\n\n\
   Ex≈cutez les ≈tapes suivantes :\n\
   1. Si vous n'avez pas de mot de passe, sautez cette ≈tape.\n\
      Si vous en avez un, vous devrez r≈pondre aux invites suivantes :\n\n\
        Modification du mot de passe associ≈ » votre code_utilisateur\n\
        Ancien mot de passe:     Entrez votre mot de passe actuel\n\
   2. Lorsque l'invite suivante s'affiche, entrez votre\n\
      nouveau mot de passe:\n\n\
        Nouveau mot de passe:\n\n\
   3. Vous devez ensuite l'entrer de nouveau » des fins de\n\
      v≈rification:\n\n\
        Entrez votre nouveau mot de passe une deuxi…me fois:\n\n\
   4. L'≈cran d'ouverture de session CUE s'affiche de\n\
      nouveau. Ouvrez une session avec le nouveau mot de passe.\n\n\
      Si vous avez besoin d'aide, adressez-vous » l'administrateur\n\
      du syst…me.
12 12 : CUE : Impossible d'ex≈cuter /bin/passwd.\n\
    Adressez-vous » l'administrateur du syst…me.
13 13 : CUE n'a pu lier les proc≈dures.  V≈rifiez\n\
    l'installation de CUE et r≈essayez.
14 14 : CUE : Impossible d'ex≈cuter /bin/cue.etc/cuesession.\n\
    Adressez-vous » l'administrateur du syst…me.
15 15 : CUE : L'ouverture de session a ≈chou≈. Le code utilisateur ou\n\
    le mot de passe est erron≈.
16 16 : CUE n'a pas pu r≈cup≈rer le message num≈ro %d dans le groupe %d\n\
    V≈rifiez que les variables environnementales LANG et NLSPATH\n\
    concordent avec le catalogue de message NLS cue.cat.\n\
    Reportez-vous aux rubriques environ(5) et lang(5) des pages de\n\
    manuel. Adressez-vous » l'administrateur du syst…me.
17 17 : CUE n'a pas suffisamment de m≈moire pour\n\
    fonctionner correctement. Adressez-vous » l'administrateur du syst…me.
18 18 : CUE ne peut lancer le shell %s. Si vous avez besoin d'aide,\n\
    adressez-vous » l'administrateur du syst…me.
19 19 : CUE n'a pas pu charger le fichier du gestionnaire d'≈crans,\n\
    /bin/cue.etc/cue.dm. V≈rifiez l'installation de CUE.
20 20 : CUE : ce type de session est introuvable sur votre syst…me.\n\
    S≈lectionnez un autre type de session.
21 21 : CUE ne peut restaurer le num≈ro d'identification de l'utilisateur.\n\
    Assurez-vous que le bit setuid est d≈fini pour /bin/cue.\n\
    Adressez-vous » l'administrateur du syst…me.
22 22 : CUE ne peut ouvrir /usr/lib/nls/config.\n\
    Adressez-vous » l'administrateur du syst…me.
23 23 : Shell restreint. Impossible d'ex≈cuter CUE » partir de la ligne de\n\
    commande.
