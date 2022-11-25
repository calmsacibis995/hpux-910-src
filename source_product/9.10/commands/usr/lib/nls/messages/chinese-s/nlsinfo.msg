$set 1
10 注释: 错误信息串
11 用法: %s [-acfhilmnstC] [-d n] [-e n] [-o n] [-r n1 n2] [-L language]\n
12 在标题之间的行数必须大于零\n
13 不能装载局域 "%s"\n
14 下界值非法\n
15 上界值非法\n
16 选项后必须跟 "o" 或 "h" 而不是 "%c"\n
17 没有安装 "/user/lib/nls/cofig"\n
18 语言 "%s" 设置了但未安装\n
19  
20 列安装语言的选项(-l)有问题\n
21 LC_ALL 选项 (-a) 有问题\n
22 LC_CTYPE 选项 (-c) 有问题\n
23 LC_COLLATE 选项 (-s) 有问题\n
24 LC_NUMERIC 选项 (-n) 有问题\n
25 LC_TIME 选项 (-t) 有问题\n
26 LC_MONETARY 选项 (-m) 有问题\n
27 其它常规选项 (-h) 有问题\n
28 列代码集选项 (-C) 有问题\n
29 列转换代码集名选项 (-i) 有问题\n
100 注释: ctype 格式字符串
101 字符分类\n
102 ========\n
103 char alpha upper lower digit xdigt alnum space punct print graph cntrl ascii \n
104 ============================================================================ \n
105 %3s%5d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d\n
200 注释: 转换格式字符串
201 字符转换\n
202 ========\n
203 char upshift downshift \n
204 ====================== \n
205 %3s%7s%9s\n
300 注释: nl_tools_16  格式字符串
301 多字节域\n
302 ========\n
303 第一字节\n
304 ========\n
305 %3s-%3s\n
306 第二字节\n
307 ========\n
400 注释: 对照单字节格式字符串
401 字符对照\n
402 ========\n
403 type ch ch1 ch2 seq seq2 pri\n
404 ============================\n
405 %3s%9s%8s%8s%8d%8s%8d\n
406 %3s%9s\n
500 注释: 对照多字节格式字符串
501 字符对照\n
502 ========\n
503 type ch ch1 ch2 seq seq2 pri\n
504 ============================\n
505 %3s%9s%8s%8s%8d%8s%8d\n
506 %3s%11s%8s%8s%7d%8s%7d\n
600 注释: lc_time格式字符串
601 时间惯用法\n
602 ==========\n
603 描述              符号            字符串\n
604 ====              ====            ======\n
605 日期时间格式串    D_T_FMT         "%s"\n
606 日期格式串        D_FMT           "%s"\n
607 时间格式串        T_FMT           "%s"\n
608 日名称            DAY_1           "%s"\n
609                   DAY_2           "%s"\n
610                   DAY_3           "%s"\n
611                   DAY_4           "%s"\n
612                   DAY_5           "%s"\n
613                   DAY_6           "%s"\n
614                   DAY_7           "%s"\n
615 缩写日名称        ABDAY_1         "%s"\n
616                   ABDAY_2         "%s"\n
617                   ABDAY_3         "%s"\n
618                   ABDAY_4         "%s"\n
619                   ABDAY_5         "%s"\n
620                   ABDAY_6         "%s"\n
621                   ABDAY_7         "%s"\n
622 月名称            MON_1           "%s"\n
623                   MON_2           "%s"\n
624                   MON_3           "%s"\n
625                   MON_4           "%s"\n
626                   MON_5           "%s"\n
627                   MON_6           "%s"\n
628                   MON_7           "%s"\n
629                   MON_8           "%s"\n
630                   MON_9           "%s"\n
631                   MON_10          "%s"\n
632                   MON_11          "%s"\n
633                   MON_12          "%s"\n
634 缩写月名称        ABMON_1         "%s"\n
635                   ABMON_2         "%s"\n
636                   ABMON_3         "%s"\n
637                   ABMON_4         "%s"\n
638                   ABMON_5         "%s"\n
639                   ABMON_6         "%s"\n
640                   ABMON_7         "%s"\n
641                   ABMON_8         "%s"\n
642                   ABMON_9         "%s"\n
643                   ABMON_10        "%s"\n
644                   ABMON_11        "%s"\n 
645                   ABMON_12        "%s"\n
646 AM字符串          AM_STR          "%s"\n
647 PM字符串          PM_STR          "%s"\n
648 年符号            YEAR_UNIT       "%s"\n
649 月符号            MON_UNIT        "%s"\n
650 日符号            DAY_UNIT        "%s"\n
651 小时符号          HOUR_UNIT       "%s"\n
652 分符号            MIN_UNIT        "%s"\n
653 秒符号            SEC_UNIT        "%s"\n
654 世纪格式字符串    REA_FMT         "%s"\n
700 注释: lc_monetary 格式字符串
701 货币习惯表示\n
702 ============\n
703 描述              符号            字串\n
704 ====              ====            ====\n
705 通货符号          CRNCYSTR        "%s"\n
706 货币lconv值\n 
707 ===========\n
708 描述               lconv 元素        值\n
709 ====               ==========       ====\n
710 国际流通符号      int_curr_symbol   "%s"\n
711 地区流通符号      currency_symbol   "%s"\n
712 货币十进制小数点  mon_decimal_point "%s"\n
713 货币千分符        mon_thousands_sep "%s"\n
714 货币数字组大小    mon_grouping      "%s"\n
715 货币正号          positive_sign     "%s"\n
716 货币负号          negative_sign     "%s"\n
717 国际小数位数      int_frac_digits  "%s"\n
718 小数位数          frac_digits       "%s"\n
719 符号在正值前后    p_cs_precedes     "%s"\n
720 空格在符号和正值之间p_sep_by_space  "%s"\n
721 符号在负值前后    n_cs_precedes     "%s"\n
722 空格在符号和负值之间n_sep_by_space  "%s"\n
723 正号位置          p_sign_posn       "%s"\n
724 负号位置          n_sign-posn       "%s"\n
800 注释: lc_numeric 格式字符串
801 数字习惯表示\n
802 ============\n
803 描述               符号           字串\n
804 ====               ====           ====\n
805 基数字符           RADIXCHAR      "%s"\n
806 千分符             THOUSEP        "%s"\n
807 替换字符           ALT_DIGIT      "%s"\n
808 数字lconv值\n      
809 ===========\n
810 描述               名称            值\n
811 ====               ====           ====\n
812 基数字符           decimal-point  "%s"\n
813 千分符             thousands_sep  "%s"\n
814 组                 grouping       "%s"\n
900 注释: 其它格式字符串
901 其它习惯表示\n
902 ============\n
903 描述               符号           字串\n
904 ====               ====           ====\n
905 肯定字符串         YESSTR         "%s"\n
906 否定字符串         NOSTR          "%s"\n
907 方向               DIRECTION      "%s"\n
1000 注释: 代码集格式字符串
1001 代码集值\n
1002 ========\n
1003 chr  hex  oct  dec \n
1004 ===================\n
1005 %1$3s%2$8x%2$8o%2$8d\n
1006 chr  hex  oct  dec \n
1007 ===================\n
1008 %1$6s%2$7x%2$9o%2$8d\n
1100 注释: list_lang 格式字符串
1101 安装的地点\n
1102 ==========\n
1103 地点               修改量\n
1104 ====               ======\n
1105 %-18s%s%s%s%s\n
1106 关于地点名称的详细情况见NLS概念和指导\n
1107 =====================================\n
1200 注释: 格式串lconv p_sign_posn 和 n_sign 值
1201 lconv p_sign_posn和n_sign值\n
1202 ===========================\n
1203 %-6s 括号把数量和流通符号括起来\n
1204 %-6s 符号在数量和流通符号之先\n
1205 %-6s 符号在数量和流通符号之后\n
1206 %-6s 符号紧接在流通符号之前\n
1207 %-6s 符号紧跟在流通符号之后\n
1208 %-6s 在当前位置信息不可用\n
1300 注释: 对 p_cs_precedes 和 n_cs_precedes 格式串
1301 lconv p_cs_precedes 和 n_cs_precedes 值\n
1302 ====================================\n
1303 %-6s 流通符号在数量之后\n
1304 %-6s 流通符号在数量之前\n
1305 %-6s 在当前位置信息不可用\n
1400 注释: 对 p_sep_by_space 和 n_sep_by_space 格式串
1401 lconv p_sep_by_space 和 n_sep_by_space 值\n
1402 ======================================\n
1403 %-6s 流通符号和数量之前不用空格分开\n
1404 %-6s 流通符号和数量之间用空格分开\n
1405 %-6s 在当前位置信息不可用\n
1500 注释: 对 grouping 和 mon_grouping 格式串
1501 lconv grouping 和 mon_grouping 值\n
1502 ==============================\n
1503 %-6s 没有进一层的分组了\n
1504 %-6s 前一个元素重复用于其余数字\n
1505 %-6s 当前组中的数字位数\n
1506 其它
1600 注释: iconv 代码集名字格式串
1601 安装的代码集转换\n
1602 ================\n
1603 Fromname          Toname\n
1604 ========          ===== \n
1605 %-20s%-20s\n
1606 名字由代码集名字的前四个和最后一个字母组成\n
1607 ==========================================\n
1700 注释: ctype_customs格式字符串
1701 Ctype Customs \n
1702 ==============\n
1703 描述               符号           字串\n
1704 ====               ====           ====\n
1705 每个字符的字节数   BYTES_CHAR     "%s"\n
1706 替换标点           ALT_PUNCT      "%s"\n
