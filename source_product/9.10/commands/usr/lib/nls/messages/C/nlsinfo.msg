$set 1
10 COMMENT: error message strings
11 usage: %s [-acfhilmnstC] [-d n] [-e n] [-o n] [-r n1 n2] [-L language]\n
12 number of lines between headings must be greater than zero\n
13 unable to load locale "%s"\n
14 invalid lower bound value\n
15 invalid upper bound value\n
16 option must be followed by "o" or "h" not "%c"\n
17 "/usr/lib/nls/config" not installed\n
18 language "%s" configured but not installed\n
19 
20 problems with list installed language option (-l)\n
21 problems with LC_ALL option (-a)\n
22 problems with LC_CTYPE option (-c)\n
23 problems with LC_COLLATE option (-s)\n
24 problems with LC_NUMERIC option (-n)\n
25 problems with LC_TIME option (-t)\n
26 problems with LC_MONETARY option (-m)\n
27 problems with other customs option (-h)\n
28 problems with list code set option (-C)\n
29 problems with list conversion code set names option (-i)\n
100 COMMENT: ctype format strings
101  Character Classification\n
102  ========================\n
103  char alpha upper lower digit xdigt alnum space punct print graph cntrl ascii\n
104  ==== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====\n
105  %3s%5d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d\n
200 COMMENT: conv format strings
201  Character Conversion\n
202  ====================\n
203  char upshift downshift\n
204  ==== ======= =========\n
205  %3s%7s%9s\n
300 COMMENT: nl_tools_16 format strings
301  Multi-byte Range\n
302  ================\n
303  First-of-two\n
304  ============\n
305    %3s - %3s\n
306  Second-of-two\n
307  =============\n
400 COMMENT: collate single-byte format strings
401  Character Collation\n
402  ===================\n
403  type      ch     ch1     ch2     seq     seq2     pri\n
404  ====     ===     ===     ===     ===     ====     ===\n
405  %3s%9s%8s%8s%8d%8s%8d\n
406  %3s%9s\n
500 COMMENT: collate multi-byte format strings
501  Character Collation\n
502  ===================\n
503  type      ch     ch1     ch2     seq     seq2     pri\n
504  ====     ===     ===     ===     ===     ====     ===\n
505  %3s%9s%8s%8s%8d%8s%8d\n
506  %3s%11s%8s%8s%7d%8s%7d\n
600 COMMENT: lc_time format strings
601  Time Customs\n
602  ============\n
603  description               symbol            string\n
604  ===========               ======            ======\n
605  date-time format string   D_T_FMT           "%s"\n
606  date format string        D_FMT             "%s"\n
607  time format string        T_FMT             "%s"\n
608  day names                 DAY_1             "%s"\n
609                            DAY_2             "%s"\n
610                            DAY_3             "%s"\n
611                            DAY_4             "%s"\n
612                            DAY_5             "%s"\n
613                            DAY_6             "%s"\n
614                            DAY_7             "%s"\n
615  abbreviated day names     ABDAY_1           "%s"\n
616                            ABDAY_2           "%s"\n
617                            ABDAY_3           "%s"\n
618                            ABDAY_4           "%s"\n
619                            ABDAY_5           "%s"\n
620                            ABDAY_6           "%s"\n
621                            ABDAY_7           "%s"\n
622  month names               MON_1             "%s"\n
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
634  abbreviated month names   ABMON_1           "%s"\n
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
646  AM string                 AM_STR            "%s"\n
647  PM string                 PM_STR            "%s"\n
648  year symbol               YEAR_UNIT         "%s"\n
649  month symbol              MON_UNIT          "%s"\n
650  day symbol                DAY_UNIT          "%s"\n
651  hour symbol               HOUR_UNIT         "%s"\n
652  minute symbol             MIN_UNIT          "%s"\n
653  second symbol             SEC_UNIT          "%s"\n
654  era format string         ERA_FMT           "%s"\n
700 COMMENT: lc_monetary format strings
701  Monetary Customs\n
702  ================\n
703  description                                     symbol               string\n
704  ===========                                     ======               ======\n
705  currency symbol                                 CRNCYSTR             "%s"\n
706  Monetary lconv Values\n
707  =====================\n
708  description                                     lconv element        value\n
709  ===========                                     =============        =====\n
710  international currency symbol                   int_curr_symbol      "%s"\n
711  local currency symbol                           currency_symbol      "%s"\n
712  monetary decimal point                          mon_decimal_point    "%s"\n
713  monetary thousands separator                    mon_thousands_sep    "%s"\n
714  size of monetary digit group                    mon_grouping         "%s"\n
715  monetary positive sign                          positive_sign        "%s"\n
716  monetary negative sign                          negative_sign        "%s"\n
717  number of international fractional digits       int_frac_digits      '%s'\n
718  number of fractional digits                     frac_digits          '%s'\n
719  symbol precedes/succeeds positive value         p_cs_precedes        '%s'\n
720  space between symbol and positive value         p_sep_by_space       '%s'\n
721  symbol precedes/succeeds negative value         n_cs_precedes        '%s'\n
722  space between symbol and negative value         n_sep_by_space       '%s'\n
723  position of positive sign                       p_sign_posn          '%s'\n
724  position of negative sign                       n_sign_posn          '%s'\n
800 COMMENT: lc_numeric format strings
801  Numeric Customs\n
802  ===============\n
803  description               symbol            string\n
804  ===========               ======            ======\n
805  radix character           RADIXCHAR         "%s"\n
806  thousands separator       THOUSEP           "%s"\n
807  alternative digits        ALT_DIGIT         "%s"\n
808  Numeric lconv Values\n
809  ====================\n
810  description               name              value\n
811  ===========               ====              =====\n
812  radix character           decimal_point     "%s"\n
813  thousands separator       thousands_sep     "%s"\n
814  grouping                  grouping          "%s"\n
900 COMMENT: other format strings
901  Other Customs\n
902  =============\n
903  description               symbol            string\n
904  ===========               ======            ======\n
905  yes string                YESSTR            "%s"\n
906  no string                 NOSTR             "%s"\n
907  direction                 DIRECTION         "%s"\n
1000 COMMENT: code set format strings
1001  Code Set Values\n
1002  ===============\n
1003  chr     hex     oct     dec\n
1004  ===========================\n
1005  %1$3s%2$8x%2$8o%2$8d\n
1006    chr     hex     oct     dec\n
1007  ===============================\n
1008  %1$6s%2$7x%2$9o%2$8d\n
1100 COMMENT: list_lang format strings
1101  Installed Locales\n
1102  =================\n
1103  Locales           Modifiers\n
1104  =======           =========\n
1105  %-18s%s%s%s%s\n
1106  See NLS Concepts and Tutorials for more information about locale names\n
1107  ======================================================================\n
1200 COMMENT: format string lconv p_sign_posn and n_sign values
1201  lconv p_sign_posn and n_sign values\n
1202  ===================================\n
1203  %-6s parentheses surround the quantity and currency_symbol\n
1204  %-6s sign precedes the quantity and currency_symbol\n
1205  %-6s sign succeeds the quantity and currency_symbol\n
1206  %-6s sign immediately precedes the currency_symbol\n
1207  %-6s sign immediately succeeds the currency_symbol\n
1208  %-6s information not available in the currrent locale\n
1300 COMMENT: format strings for p_cs_precedes and n_cs_precedes
1301  lconv p_cs_precedes and n_cs_precedes values\n
1302  ============================================\n
1303  %-6s currency_symbol succeeds quantity\n
1304  %-6s currency_symbol precedes quantity\n
1305  %-6s information not available in the currrent locale\n
1400 COMMENT: format string for p_sep_by_space and n_sep_by_space
1401  lconv p_sep_by_space and n_sep_by_space values\n
1402  ==============================================\n
1403  %-6s currency_symbol is not separated by a space from quantity\n
1404  %-6s currency_symbol is separated by a space from quantity\n
1405  %-6s information not available in the currrent locale\n
1500 COMMENT: format strings for grouping and mon_grouping
1501  lconv grouping and mon_grouping values\n
1502  ======================================\n
1503    %-6s no further grouping\n
1504    %-6s previous element repeatedly used for remainder of digits\n
1505    %-6s number of digits in the current group\n
1506 other
1600 COMMENT: iconv codeset name format strings
1601  Installed Codeset Conversions\n
1602  =============================\n
1603  Fromname            Toname\n
1604  ========            ======\n
1605  %-20s%-20s\n
1606  Names made up of first four and last letters of the code set name\n
1607  =================================================================\n
1700 COMMENT: ctype_customs format strings
1701  Ctype Customs\n
1702  =============\n
1703  description               symbol            string\n
1704  ===========               ======            ======\n
1705  bytes per character       BYTES_CHAR        "%s"\n
1706  alternative punctuation   ALT_PUNCT         "%s"\n
