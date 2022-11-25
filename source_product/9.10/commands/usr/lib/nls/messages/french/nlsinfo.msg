$set 1
10 COMMENTAIRE: cha—ne de messages d'erreur
11 syntaxe: %s [-acfhilmnstC] [-d n] [-e n] [-o n] [-r n1 n2] [-L langue]\n
12 le nombre de lignes entre les en-t¡tes doit ¡tre sup≈rieur » z≈ro\n
13 impossible de charger les langues nationales "%s"\n
14 valeur de la borne inf≈rieure non valide\n
15 valeur de la borne sup≈rieure non valide\n
16 l'option doit ¡tre suivie de "o" ou "h", et non "%c"\n
17 "/usr/lib/nls/config" n'est pas install≈\n
18 la langue "%s" est configur≈e mais pas install≈e\n
19 
20 erreur dans liste des langues install≈es; option (-l)\n
21 erreur dans LC_ALL option (-a)\n
22 erreur dans LC_CTYPE option (-c)\n
23 erreur dans LC_COLLATE option (-s)\n
24 erreur dans LC_NUMERIC option (-n)\n
25 erreur dans LC_TIME option (-t)\n
26 erreur dans LC_MONETARY option (-m)\n
27 erreur dans autres informations de personnalisation option (-h)\n
28 erreur dans la liste des jeux de caract…res option (-C)\n
29 erreur dans liste de conversion des jeux de caract…res option (-i)\n
100 COMMENTAIRE: format des cha—nes de type de caract…re
101  Classification de caract…res\n
102  ============================\n
103  car alpha majus minus num≈r xnum alnum espac ponct impr graph ctrl ascii\n
104  === ===== ===== ===== ===== ==== ===== ===== ===== ==== ===== ==== =====\n
105  %3s%5d%6d%6d%6d%5d%6d%6d%6d%5d%6d%5d%6d\n
200 COMMENTAIRE: format des cha—nes de conversion
201  Conversion de caract…res\n
202  ========================\n
203  caract…re mise en majusc mise en minusc\n
204  ========= ============== ==============\n
205  %5s%12s%15s\n
300 COMMENTAIRE: format des cha—nes nl_tools_16
301  Plage caract…re » octets multiples\n
302  ==================================\n
303  Premier de deux\n
304  ===============\n
305    %3s - %3s\n
306  Deuxi…me de deux\n
307  ================\n
400 COMMENTAIRE: format des cha—nes de classement des caract…res » un octet
401  Interclassement des caract…res\n
402  ==============================\n
403  type     car     car1     car2     s≈q     s≈q2     pri\n
404  ====     ===     ====     ====     ===     ====     ===\n
405  %3s%9s%8s%9s%9d%8s%9d\n
406  %3s%9s\n
500 COMMENTAIRE:format cha—nes de classement des caract…res » octets multiples
501  Interclassement des caract…res\n
502  ==============================\n
503  type     car     car1     car2     s≈q     s≈q2     pri\n
504  ====     ===     ====     ====     ===     ====     ===\n
505  %3s%9s%8s%9s%9d%8s%9d\n
506  %3s%11s%8s%9s%8d%8s%8d\n
600 COMMENTAIRE: format des cha—nes d'horodatage
601  Horodatage\n
602  ==========\n
603  description               symbole           cha—ne\n
604  ===========               =======           ======\n
605  format de la date/heure   D_T_FMT           "%s"\n
606  format de la date         D_FMT             "%s"\n
607  format de l'heure         T_FMT             "%s"\n
608  noms des jours            DAY_1             "%s"\n
609                            DAY_2             "%s"\n
610                            DAY_3             "%s"\n
611                            DAY_4             "%s"\n
612                            DAY_5             "%s"\n
613                            DAY_6             "%s"\n
614                            DAY_7             "%s"\n
615  noms abr≈g≈s des jours    ABDAY_1           "%s"\n
616                            ABDAY_2           "%s"\n
617                            ABDAY_3           "%s"\n
618                            ABDAY_4           "%s"\n
619                            ABDAY_5           "%s"\n
620                            ABDAY_6           "%s"\n
621                            ABDAY_7           "%s"\n
622  noms des mois             MON_1             "%s"\n
623                            MON_2             "%s"\n
624                            MON_3             "%s"\n
625                            MON_4             "%s"\n
626                            MON_5             "%s"\n
627                            MON_6             "%s"\n
628                            MON_7             "%s"\n
629                            MON_8             "%s"\n
630                            MON_9             "%s"\n
631                            MON_10            "%s"\n
632                            MON_11            "%s"\n
633                            MON_12            "%s"\n
634  noms abr≈g≈s des mois     ABMON_1           "%s"\n
635                            ABMON_2           "%s"\n
636                            ABMON_3           "%s"\n
637                            ABMON_4           "%s"\n
638                            ABMON_5           "%s"\n
639                            ABMON_6           "%s"\n
640                            ABMON_7           "%s"\n
641                            ABMON_8           "%s"\n
642                            ABMON_9           "%s"\n
643                            ABMON_10          "%s"\n
644                            ABMON_11          "%s"\n
645                            ABMON_12          "%s"\n
646  cha—ne pour le matin      AM_STR            "%s"\n
647  cha—ne pour l'apr…s-midi  PM_STR            "%s"\n
648  symbole de l'ann≈e        YEAR_UNIT         "%s"\n
649  symbole du mois           MON_UNIT          "%s"\n
650  symbole du jour           DAY_UNIT          "%s"\n
651  symbole de l'heure        HOUR_UNIT         "%s"\n
652  symbole des minutes       MIN_UNIT          "%s"\n
653  symbole des secondes      SEC_UNIT          "%s"\n
654  format de l'…re           ERA_FMT           "%s"\n
700 COMMENTAIRE: format des cha—nes des unit≈s mon≈taires
701  Unit≈s mon≈taires\n
702  =================\n
703  description                                    symbole              cha—ne\n
704  ===========                                    =======              ======\n
705  symbole mon≈taire                              CRNCYSTR             "%s"\n
706  Valeurs de conversion des monnaies\n
707  ==================================\n
708  description                                    ≈l≈ment lconv        valeur\n
709  ===========                                    =============        ======\n
710  unit≈ mon≈taire dans le syst…me international  int_curr_symbol      "%s"\n
711  symbole de l'unit≈ mon≈taire locale            currency_symbol      "%s"\n
712  d≈cimale de l'unit≈ mon≈taire                  mon_decimal_point    "%s"\n
713  s≈parateur de milliers de l'unit≈              mon_thousands_sep    "%s"\n
714  unit≈ mon≈taire - groupe de nombres            mon_grouping         "%s"\n
715  signe plus (unit≈ mon≈taire)                   positive_sign        "%s"\n
716  signe moins (unit≈ mon≈taire)                  negative_sign        "%s"\n
717  nombre de d≈cimales dans le syst…me internat   int_frac_digits      '%s'\n
718  nombre de d≈cimales                            frac_digits          '%s'\n
719  symbole avant/apr…s la valeur positive         p_cs_precedes        '%s'\n
720  espace entre le symbole et la valeur positive  p_sep_by_space       '%s'\n
721  symbole avant/apr…s la valeur n≈gative         n_cs_precedes        '%s'\n
722  espace entre le symbole et la valeur n≈gative  n_sep_by_space       '%s'\n
723  position du signe plus                         p_sign_posn          '%s'\n
724  position du signe moins                        n_sign_posn          '%s'\n
800 COMMENTAIRE: format des cha—nes des valeurs num≈riques
801  Personnalisations des nombres\n
802  =============================\n
803  description               symbole           cha—ne\n
804  ===========               =======           ======\n
805  signe d≈cimal             RADIXCHAR         "%s"\n
806  s≈parateur de milliers    THOUSEP           "%s"\n
807  chiffres alternatifs      ALT_DIGIT         "%s"\n
808  Valeurs num≈riques de conversion\n
809  ================================\n
810  description               nom               valeur\n
811  ===========               ===               ======\n
812  signe d≈cimal             decimal_point     "%s"\n
813  s≈parateur de milliers    thousands_sep     "%s"\n
814  groupe de nombres         grouping          "%s"\n
900 COMMENTAIRE: format des autres cha—nes
901  Autres personnalisations\n
902  ========================\n
903  description               symbole           cha—ne\n
904  ===========               =======           ======\n
905  cha—ne du oui             YESSTR            "%s"\n
906  cha—ne du non             NOSTR             "%s"\n
907  direction                 DIRECTION         "%s"\n
1000 COMMENTAIRE: format des cha—nes du jeu de caract…res
1001  Valeurs du jeu de caract…res\n
1002  ============================\n
1003  car     hex     oct     d≈c\n
1004  ============================\n
1005  %1$3s%2$8x%2$8o%2$8d\n
1006    car     hex     oct     d≈c\n
1007  ===============================\n
1008  %1$6s%2$7x%2$9o%2$8d\n
1100 COMMENTAIRE: format des cha—nes de la liste des langues
1101  Langues nationales install≈es\n
1102  =============================\n
1103  Langues nationales       Modificateurs\n
1104  ==================       =============\n
1105  %-25s%s%s%s%s\n
1106  Voir NLS Concepts and Tutorials pour noms langues nationales\n
1107  ============================================================\n
1200 COMMENTAIRE: format des cha—nes lconv p_sign_posn et n_sign
1201  valeurs lconv de p_sign_posn et n_sign\n
1202  ======================================\n
1203  %-6s parenth…ses entourent nombre et symbole de l'unit≈\n
1204  %-6s signe pr≈c…de le nombre et le symbole de l'unit≈\n
1205  %-6s signe suit le nombre et le symbole de l'unit≈ mon≈taire\n
1206  %-6s signe pr≈c…de imm≈diatement le symbole de la monnaie\n
1207  %-6s signe suit imm≈diatement le symbole de la monnaie\n
1208  %-6s information non disponible dans la langue nationale\n
1300 COMMENTAIRE: format des cha—nes p_cs_precedes et n_cs_precedes
1301  valeurs lconv de p_cs_precedes et n_cs_precedes\n
1302  ===============================================\n
1303  %-6s symbole de l'unit≈ mon≈taire suit le nombre\n
1304  %-6s symbole de l'unit≈ mon≈taire pr≈c…de le nombre\n
1305  %-6s information non disponible dans la langue nationale\n
1400 COMMENTAIRE: format des cha—nes p_sep_by_space et n_sep_by_space
1401  valeurs lconv de p_sep_by_space et n_sep_by_space\n
1402  =================================================\n
1403  %-6s symbole de l'unit≈ mon≈taire non s≈par≈ du nombre par un espace\n
1404  %-6s le symbole de monnaie est s≈par≈ du nombre par un espace\n
1405  %-6s l'information non disponible dans langue nationale\n
1500 COMMENTAIRE: format des cha—nes de grouping et des mon_grouping
1501  valeurs lconv de grouping et de mon_grouping\n
1502  ============================================\n
1503    %-6s plus de regroupement\n
1504    %-6s ≈l≈ment pr≈c≈dent utilis≈ » r≈p≈tition pour les chiffres restants\n
1505    %-6s nombre de chiffres dans le groupe courant\n
1506 autre
1600 COMMENTAIRE: format des cha—nes iconv codeset name
1601  Conversions des jeux de caract…res install≈s\n
1602  ============================================\n
1603  Du nom              Au nom\n
1604  ======              ======\n
1605  %-20s%-20s\n
1606  Noms compos≈s des quatre premi…res et de la derni…re lettre du jeu de caract\n
1607  ============================================================================\n
1700 COMMENTAIRE: format des cha—nes ctype_customs
1701  Personnalisations type de caract…res (Ctype)\n
1702  ============================================\n
1703  description               symbole           cha—ne\n
1704  ===========               =======           ======\n
1705  octets par caract…re      BYTES_CHAR        "%s"\n
1706  ponctuation alternative   ALT_PUNCT         "%s"\n
