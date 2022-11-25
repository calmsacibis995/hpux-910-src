$set 1
10 COMMENT: エラー・メッセージ文字列
11 使用法: %s [-acfhilmnstC] [-d n] [-e n] [-o n] [-r n1 n2] [-L language]\n
12 ヘッディング間のライン数は０以上でなければなりません。\n
13 ローカル "%s" をロードできません。\n
14 下限値が正しくありません。\n
15 上限値が正しくありません。\n
16 オプションは "o" や "h" の後には付きますが "%c" の後には付きません。\n
17 "/usr/lib/nls/config" がインストールされていません。\n
18 "%s" はコンフィギュレートされていますがインストールされていません。\n
19 
20 インストールされている言語をリストするオプション (-l) で問題が発生しました。\n
21 LC_ALL オプション (-a) で問題が発生しました。\n
22 LC_CTYPE オプション (-c) で問題が発生しました。\n
23 LC_COLLATE オプション (-s) で問題が発生しました。\n
24 LC_NUMERIC オプション (-n) で問題が発生しました。\n
25 LC_TIME オプション (-t) で問題が発生しました。\n
26 LC_MONETARY オプション (-m) で問題が発生しました。\n
27 その他の習慣を調べるオプション (-h) で問題が発生しました。\n
28 コード・セットをリストするオプション (-C) で問題が発生しました。\n
29 変換コード・セット名をリストするオプション (-i) で問題が発生しました。\n
100 COMMENT: ctype フォーマット文字列
101 文字の分類\n
102  ========================\n
103  char alpha upper lower digit xdigt alnum space punct print graph cntrl ascii\n
104  ==== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====\n
105  %3s%5d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d\n
200 COMMENT: conv フォーマット文字列
201 文字の変換\n
202  ====================\n
203  char upshift downshift\n
204  ==== ======= =========\n
205  %3s%7s%9s\n
300 COMMENT: nl_tools_16 フォーマット文字列
301 マルチ・バイトの範囲\n
302  ================\n
303  First-of-two\n
304  ============\n
305    %3s - %3s\n
306  Second-of-two\n
307  =============\n
400 COMMENT: シングル・バイト照合フォーマット文字列
401 文字の照合\n
402  ===================\n
403  type      ch     ch1     ch2     seq     seq2     pri\n
404  ====     ===     ===     ===     ===     ====     ===\n
405  %3s%9s%8s%8s%8d%8s%8d\n
406  %3s%9s\n
500 COMMENT: マルチ・バイト照合フォーマット文字列
501 文字の照合\n
502  ===================\n
503  type      ch     ch1     ch2     seq     seq2     pri\n
504  ====     ===     ===     ===     ===     ====     ===\n
505  %3s%9s%8s%8s%8d%8s%8d\n
506  %3s%11s%8s%8s%7d%8s%7d\n
600 COMMENT: lc_time フォーマット文字列
601 時間表記の習慣\n
602  ============\n
603  description               symbol            string\n
604  ===========               ======            ======\n
605  日時 のフォーマット       D_T_FMT           "%s"\n
606  日付の文字列              D_FMT             "%s"\n
607  時刻の文字列              T_FMT             "%s"\n
608  週日名                    DAY_1             "%s"\n
609                            DAY_2             "%s"\n
610                            DAY_3             "%s"\n
611                            DAY_4             "%s"\n
612                            DAY_5             "%s"\n
613                            DAY_6             "%s"\n
614                            DAY_7             "%s"\n
615  週日名の省略形            ABDAY_1           "%s"\n
616                            ABDAY_2           "%s"\n
617                            ABDAY_3           "%s"\n
618                            ABDAY_4           "%s"\n
619                            ABDAY_5           "%s"\n
620                            ABDAY_6           "%s"\n
621                            ABDAY_7           "%s"\n
622  月名                      MON_1             "%s"\n
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
634  月名の省略形              ABMON_1           "%s"\n
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
646  午前の表記                AM_STR            "%s"\n
647  午後の表記                PM_STR            "%s"\n
648  年の記号                  YEAR_UNIT         "%s"\n
649  月の記号                  MON_UNIT          "%s"\n
650  日の記号                  DAY_UNIT          "%s"\n
651  時の記号                  HOUR_UNIT         "%s"\n
652  分の記号                  MIN_UNIT          "%s"\n
653  秒の記号                  SEC_UNIT          "%s"\n
654  元号の表記                ERA_FMT           "%s"\n
700 COMMENT: lc_monetary フォーマット文字列
701 通貨表記の習慣\n
702  ================\n
703  description                                     symbol               string\n
704  ===========                                     ======               ======\n
705  通貨記号                                        CRNCYSTR             "%s"\n
706 通貨変換値\n
707  =====================\n
708  description                                     lconv element        value\n
709  ===========                                     =============        =====\n
710  国際通貨記号                                    int_curr_symbol      "%s"\n
711  国内通貨記号                                    currency_symbol      "%s"\n
712  通貨用小数点                                    mon_decimal_point    "%s"\n
713  通貨用桁区切り記号                              mon_thousands_sep    "%s"\n
714  通貨数字グループの大きさ                        mon_grouping         "%s"\n
715  通貨用正記号                                    positive_sign        "%s"\n
716  通貨用負記号                                    negative_sign        "%s"\n
717  国際市場における小数桁数                        int_frac_digits      '%s'\n
718  国内市場における小数桁数                        frac_digits          '%s'\n
719  正値に対する通貨記号の位置                      p_cs_precedes        '%s'\n
720  正値と通貨記号の間の空白の有無                  p_sep_by_space       '%s'\n
721  負値に対する通貨記号の位置                      n_cs_precedes        '%s'\n
722  負値と通貨記号の間の空白の有無                  n_sep_by_space       '%s'\n
723  正記号の位置                                    p_sign_posn          '%s'\n
724  負記号の位置                                    n_sign_posn          '%s'\n
800 COMMENT: lc_numeric フォーマット文字列
801 文字表記の習慣\n
802  ===============\n
803  description               symbol            string\n
804  ===========               ======            ======\n
805  小数点用文字              RADIXCHAR         "%s"\n
806  桁区切り用文字            THOUSEP           "%s"\n
807  代替用数字                ALT_DIGIT         "%s"\n
808 数字変換値\n
809  ====================\n
810  description               name              value\n
811  ===========               ====              =====\n
812  小数点用文字              decimal_point     "%s"\n
813  桁区切り用文字            thousands_sep     "%s"\n
814  数字グループ              grouping          "%s"\n
900 COMMENT: その他のフォーマット文字列
901 その他の習慣\n
902  =============\n
903  description               symbol            string\n
904  ===========               ======            ======\n
905  yes 文字列                YESSTR            "%s"\n
906  no 文字列                 NOSTR             "%s"\n
907  表記方向                  DIRECTION         "%s"\n
1000 COMMENT: コード・セット・フォーマット文字列
1001 コード・セットの値\n
1002  ===============\n
1003  chr     hex     oct     dec\n
1004  ===========================\n
1005  %1$3s%2$8x%2$8o%2$8d\n
1006    chr     hex     oct     dec\n
1007  ===============================\n
1008  %1$6s%2$7x%2$9o%2$8d\n
1100 COMMENT: list_lang フォーマット文字列
1101 インストールされている言語\n
1102  =================\n
1103  Locales           Modifiers\n
1104  =======           =========\n
1105  %-18s%s%s%s%s\n
1106 言語名に関する詳細については NLS Concepts and Tutorials を参照して下さい。\n
1107  ======================================================================\n
1200 COMMENT: フォーマット文字列 p_sign_posn と n_sign の変換値
1201 p_sign_posn と n_sign の変換値\n
1202  ===================================\n
1203  カッコ %-6s が値と通貨記号を囲みます。\n
1204  符号 %-6s が値と通貨記号の前に付きます。\n
1205  符号 %-6s が値と通貨記号の後に付きます。\n
1206  符号 %-6s が通貨記号の直前に付きます。\n
1207  符号 %-6s が通貨記号の直後に付きます。\n
1208  %-6s 情報は現在の言語では使用できません。\n
1300 COMMENT: p_cs_precedes と n_cs_precedes のためのフォーマット文字列
1301 p_cs_precedes と n_precedes の変換値\n
1302  ============================================\n
1303  通貨記号 %-6s は値の後に付きます。\n
1304  通貨記号 %-6s は値の前に付きます。\n
1305  %-6s 情報は現在の言語では使用できません。\n
1400 COMMENT: p_sep_by_space と n_sep_by_space のためのフォーマット文字列
1401 p_sep_by_space と n_sep_by_space の変換値\n
1402  ==============================================\n
1403  通貨記号 %-6s は値との間を空白で区切りません。\n
1404  通貨記号 %-6s は値との間を空白で区切ります。\n
1405  %-6s 情報は現在の言語では使用できません。\n
1500 COMMENT: grouping と mon_grouping のためのフォーマット文字列
1501 grouping と mon_grouping の変換値\n
1502  ======================================\n
1503  %-6s はグルーピングを行いません。\n
1504  %-6s は前の要素を残りの数字に対しても繰り返し使います。\n
1505  %-6s 個の数字が現在グループになっています。\n
1506  その他
1600 COMMENT: コード・セット名変換フォーマット文字列
1601 インストールされたコード・セットの変換\n
1602  =============================\n
1603  Fromname            Toname\n
1604  ========            ======\n
1605  %-20s%-20s\n
1606  名前はコード・セット名の最初の４文字と最後の文字から構成されています。\n
1607  =================================================================\n
1700 COMMENT: ctype_customs フォーマット文字列
1701 Ctype の習慣\n
1702  =============\n
1703  description               symbol            string\n
1704  ===========               ======            ======\n
1705  １文字当りのバイト数      BYTES_CHAR        "%s"\n
1706  代替句読点                ALT_PUNCT         "%s"\n
