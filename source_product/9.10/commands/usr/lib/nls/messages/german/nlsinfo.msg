$set 1
10 KOMMENTAR: Fehlermeldungszeichenketten
11 Syntax: %s [-acfhilmnstC] [-d n] [-e n] [-o n] [-r n1 n2] [-L Sprache]\n
12 Anzahl der Zeilen zwischen €berschriften muﬁ grŒﬁer als Null sein\n
13 "%s" kann nicht geladen werden\n
14 Fehlerhafter unterer Grenzwert\n
15 Fehlerhafter oberer Grenzwert\n
16 Auf die Option muﬁ "o" oder "h" und nicht "%c" folgen\n
17 "/usr/lib/nls/config" nicht installiert\n
18 Sprache "%s" konfiguriert, aber nicht installiert\n
19 
20 Probleme mit Option zur Auflistung installierter Sprachen (-l)\n
21 Probleme mit LC_ALL-Option (-a)\n
22 Probleme mit LC_CTYPE-Option (-c)\n
23 Probleme mit LC_COLLATE-Option (-s)\n
24 Probleme mit LC_NUMERIC-Option (-n)\n
25 Probleme mit LC_TIME-Option (-t)\n
26 Probleme mit LC_MONETARY-Option (-m)\n
27 Probleme mit Option fœr sonstige Besonderheiten (-h)\n
28 Probleme mit Option zur Auflistung der verschiedenen Codes (-C)\n
29 Probleme mit Option zur Auflistung der zu konvertierenden Code-Paare (-i)\n
100 KOMMENTAR: Zeichenketten im "ctype"-Format
101  Zeichenklassifizierung\n
102  ======================\n
103  char alpha upper lower digit xdigt alnum space punct print graph cntrl ascii\n
104  ==== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====\n
105  %3s%5d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d\n
200 KOMMENTAR: Zeichenketten im Konvertierungsformat
201  Zeichenkonvertierung\n
202  ====================\n
203  char upshift downshift\n
204  ==== ======= =========\n
205  %3s%7s%9s\n
300 KOMMENTAR: Zeichenketten im "nl_tools_16"-Format
301  Multi-Byte-Bereich\n
302  ==================\n
303  Eins-von-zwei\n
304  =============\n
305    %3s - %3s\n
306  Zwei-von-zwei\n
307  =============\n
400 KOMMENTAR: Vergleich von Zeichenketten im Ein-Byte-Format
401  Zeichenvergleich\n
402  ================\n
403  Typ      ch     ch1     ch2     seq     seq2     pri\n
404  ====     ===     ===     ===     ===     ====     ===\n
405  %3s%9s%8s%8s%8d%8s%8d\n
406  %3s%9s\n
500 KOMMENTAR: Vergleich von Zeichenketten im Mehr-Byte-Format
501  Zeichenvergleich\n
502  ================\n
503  Typ      ch     ch1     ch2     seq     seq2     pri\n
504  ====     ===     ===     ===     ===     ====     ===\n
505  %3s%9s%8s%8s%8d%8s%8d\n
506  %3s%11s%8s%8s%7d%8s%7d\n
600 KOMMENTAR: Zeichenketten im "lc_time"-Format
601  Zeitkonventionen\n
602  ==================\n
603  Beschreibung              Symbol            Zeichenkette\n
604  ============              ======            ============\n
605  Datum-Zeit-Format         D_T_FMT           "%s"\n
606  Datum-Format              D_FMT             "%s"\n
607  Zeit-Format               T_FMT             "%s"\n
608  Tagesnamen                DAY_1             "%s"\n
609                            DAY_2             "%s"\n
610                            DAY_3             "%s"\n
611                            DAY_4             "%s"\n
612                            DAY_5             "%s"\n
613                            DAY_6             "%s"\n
614                            DAY_7             "%s"\n
615  Tagesnamen/Abkœrz.        ABDAY_1           "%s"\n
616                            ABDAY_2           "%s"\n
617                            ABDAY_3           "%s"\n
618                            ABDAY_4           "%s"\n
619                            ABDAY_5           "%s"\n
620                            ABDAY_6           "%s"\n
621                            ABDAY_7           "%s"\n
622  Monatsnamen               MON_1             "%s"\n
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
634  Monatsnamen/Abkœrz.       ABMON_1           "%s"\n
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
646  AM-Darstellung            AM_STR            "%s"\n
647  PM-Darstellung            PM_STR            "%s"\n
648  Jahr-Symbol               YEAR_UNIT         "%s"\n
649  Monat-Symbol              MON_UNIT          "%s"\n
650  Tag-Symbol                DAY_UNIT          "%s"\n
651  Stunde-Symbol             HOUR_UNIT         "%s"\n
652  Minute-Symbol             MIN_UNIT          "%s"\n
653  Sekunde-Symbol            SEC_UNIT          "%s"\n
654  "era"-Format              ERA_FMT           "%s"\n
700 KOMMENTAR: Zeichenketten im "lc_monetary"-Format
701  WÃhrungskonventionen\n
702  ====================\n
703  Beschreibung                                    Symbol               Zeichenkette\n
704  ============                                    ======               ============\n
705  WÃhrungssymbol                                  CRNCYSTR             "%s"\n
706  WÃhrungskonvertierungswerte\n
707  ===========================\n
708  Beschreibung                                    lconv Element        Wert\n
709  ============                                    =============        ====\n
710  Internationales WÃhrungssymbol                  int_curr_symbol      "%s"\n
711  Lokales WÃhrungssymbol                          currency_symbol      "%s"\n
712  WÃhrung / Dezimalpunkt                          mon_decimal_point    "%s"\n
713  WÃhrung / Tausender-Trennzeichen                mon_thousands_sep    "%s"\n
714  GrŒﬁe der WÃhrungszifferngruppe                 mon_grouping         "%s"\n
715  WÃhrung / positives Vorzeichen                  positive_sign        "%s"\n
716  WÃhrung / negatives Vorzeichen                  negative_sign        "%s"\n
717  Anzahl internationaler Bruchziffern             int_frac_digits      '%s'\n
718  Anzahl Bruchziffern                             frac_digits          '%s'\n
719  Symbol vor/nach positivem Wert                  p_cs_precedes        '%s'\n
720  Platz zwischen Symbol und positivem Wert        p_sep_by_space       '%s'\n
721  Symbol vor/nach negativem Wert                  n_cs_precedes        '%s'\n
722  Platz zwischen Symbol und negativem Wert        n_sep_by_space       '%s'\n
723  Position des positiven Vorzeichens              p_sign_posn          '%s'\n
724  Position des negativen Vorzeichens              n_sign_posn          '%s'\n
800 KOMMENTAR: Zeichenketten im "lc_numeric"-Format
801  Numerische Konventionen\n
802  =========================\n
803  Beschreibung              Symbol            Zeichenkette\n
804  ============              ======            ============\n
805  Radix-Zeichen             RADIXCHAR         "%s"\n
806  Tausend-Trennsymbol       THOUSEP           "%s"\n
807  Alternativzeichen         ALT_DIGIT         "%s"\n
808  Numerische Konvertierungswerte\n
809  ==============================\n
810  Beschreibung              Name              Wert\n
811  ============              ====              ====\n
812  Radix-Zeichen             decimal_point     "%s"\n
813  Tausend-Trennsymbol       thousands_sep     "%s"\n
814  Gruppierung               grouping          "%s"\n
900 KOMMENTAR: Zeichenketten in sonstigen Formaten
901  Sonstige Konventionen\n
902  =======================\n
903  Beschreibung              Symbol            Zeichenkette\n
904  ============              ======            ============\n
905  "yes"-Zeichenkette        YESSTR            "%s"\n
906  "no" -Zeichenkette        NOSTR             "%s"\n
907  Richtung                  DIRECTION         "%s"\n
1000 KOMMENTAR: Zeichenkette im "Code"-Format
1001  Code-Bereichswerte\n
1002  ==================\n
1003  chr     hex     oct     dec\n
1004  ===========================\n
1005  %1$3s%2$8x%2$8o%2$8d\n
1006    chr     hex     oct     dec\n
1007  ===============================\n
1008  %1$6s%2$7x%2$9o%2$8d\n
1100 KOMMENTAR: Zeichenketten im "list_lang"-Format
1101  Installierte Lokalisierungseinrichtungen\n
1102  ========================================\n
1103  Lokalisierung     Bezeichner\n
1104  =============     ==========\n
1105  %-18s%s%s%s%s\n
1106  Siehe "NLS Concepts und Tutorials" fœr weitere Informationen œber Lokalisisierungsbereichsnamen\n
1107  ===============================================================================================\n
1200 KOMMENTAR: Zeichenketten im "p_sign_posn"- und "n_sign"-Konvertierungsformat
1201  Konvertierungswerte von p_sign_posn und n_sign\n
1202  ==============================================\n
1203  %-6s Klammern um die Menge und das WÃhrungssymbol\n
1204  %-6s Vorzeichen steht vor der Menge und dem WÃhrungssymbol\n
1205  %-6s Vorzeichen steht hinter der Menge und dem WÃhrungssymbol\n
1206  %-6s Vorzeichen steht unmittelbar vor dem WÃhrungssymbol\n
1207  %-6s Vorzeichen steht unmittelbar nach dem WÃhrungssymbol\n
1208  %-6s Information steht nicht im aktuellen Lokalisierungsbereich zur Verfœgung\n
1300 KOMMENTAR: Zeichenketten im "p_cs_precedes"- und "n_cs_precedes"-Format
1301  Konvertierungswerte von p_cs_precedes und n_cs_precedes\n
1302  =======================================================\n
1303  %-6s WÃhrungssymbol steht nach Menge\n
1304  %-6s WÃhrungssymbol steht vor Menge\n
1305  %-6s Information steht nicht im aktuellen Lokalisierungsbereich zur Verfœgung\n
1400 KOMMENTAR: Zeichenketten im "p_sep_by_space"- und "n_sep_by_space"-Format
1401  Konvertierungswerte von p_sep_by_space und n_sep_by_space\n
1402  =========================================================\n
1403  %-6s WÃhrungssymbol ist nicht durch Leerzeichen von der Menge getrennt\n
1404  %-6s WÃhrungssymbol ist durch Leerzeichen von der Menge getrennt\n
1405  %-6s Information steht nicht im aktuellen Lokalisierungsbereich zur Verfœgung\n
1500 KOMMENTAR: Zeichenketten im "grouping"- und "mon_grouping"-Format
1501  Konvertierungswerte von grouping und mon_grouping\n
1502  =================================================\n
1503    %-6s Keine weitere Gruppierung\n
1504    %-6s Vorheriges Element wiederholt fœr restliche Ziffern verwendet\n
1505    %-6s Anzahl der Ziffern in der aktuellen Gruppe\n
1506 andere
1600 KOMMENTAR: Zeichenketten im "iconv codeset name"-Format
1601  Installierte Code-Konvertierungen\n
1602  =================================\n
1603  Von-Name            Nach-Name\n
1604  ========            =========\n
1605  %-20s%-20s\n
1606  Namen, aus den ersten vier und den letzten Buchstaben des Code-Namen zusammengesetzt\n
1607  ====================================================================================\n
1700 KOMMENTAR: Zeichenketten im "ctype_customs"-Format
1701  Ctype-Konventionen\n
1702  ====================\n
1703  Beschreibung                 Symbol            Zeichenkette\n
1704  ============                 ======            ============\n
1705  Bytes pro Zeichen            BYTES_CHAR        "%s"\n
1706  Alternativinterpunktion      ALT_PUNCT         "%s"\n
