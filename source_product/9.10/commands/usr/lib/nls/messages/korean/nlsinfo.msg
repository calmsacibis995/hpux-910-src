$set 1
10 COMMENT: 오류 메세지 문자열
11 사용법: %s [-acfhilmnstC] [-d n] [-e n] [-o n] [-r n1 n2] [-L language]\n
12 제목 사이의 행수는 0보다 커야 합니다.\n
13 장소 "%s"을(를) 로드할 수 없습니다.\n
14 하위 경계가 잘못되었습니다.\n
15 상위 경계가 잘못되었습니다.\n
16 선택항목 뒤에는 "o" 또는 "h"가 필요하며, "%c"을(를) 지정할 수 없습니다.\n
17 "/usr/lib/nls/config"가 설치되지 않았습니다.\n
18 언어 "%s"이(가) 구성은 되어 있으나 설치되지 않았습니다.\n
19   
20 설치된 언어의 일람 선택항목(-l)의 문제입니다.\n
21 LC_ALL 선택항목(-a)의 문제입니다.\n
22 LC_CTYPE 선택항목(-c)의 문제입니다.\n
23 LC_COLLATE 선택항목(-s)의 문제입니다.\n
24 LC_NUMERIC 선택항목(-n)의 문제입니다.\n
25 LC_TIME 선택항목(-t)의 문제입니다.\n
26 LC_MONETARY 선택항목(-m)의 문제입니다.\n
27 기타 맞춤(customs) 선택항목(-h)의 문제입니다.\n
28 코드 세트 일람 선택항목(-C)의 문제입니다.\n
29 변환 코드 세트명 일람 선택항목(-i)의 문제입니다.\n
100 COMMENT: ctype 포맷 문자열
101  문자 분류\n
102  =========\n
103  char alpha upper lower digit xdigt alnum space punct print graph cntrl ascii\n
104  ==== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====\n
105  %3s%5d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d\n
200 COMMENT: 변환 포맷 문자열
201  문자 변환\n
202  =========\n
203  char upshift downshift\n
204  ==== ======= =========\n
205  %3s%7s%9s\n
300 COMMENT: nl_tools_16 포맷 문자열
301  복수 바이트 범위\n
302  ================\n
303  2개 중 첫번째\n
304  ============\n
305    %3s - %3s\n
306  2개 중 두번째\n
307  =============\n
400 COMMENT: 단일-바이트 포맷 문자열의 배열
401  문자 배열\n
402  =========\n
403  type      ch     ch1     ch2     seq     seq2     pri\n
404  ====     ===     ===     ===     ===     ====     ===\n
405  %3s%9s%8s%8s%8d%8s%8d\n
406  %3s%9s\n
500 COMMENT: 복수-바이트 포맷 문자열의 배열
501  문자 배열\n
502  =========\n
503  type      ch     ch1     ch2     seq     seq2     pri\n
504  ====     ===     ===     ===     ===     ====     ===\n
505  %3s%9s%8s%8s%8d%8s%8d\n
506  %3s%11s%8s%8s%7d%8s%7d\n
600 COMMENT: lc_time 포맷 문자열
601  시간 맞춤(Custom)\n
602  =================\n
603  설       명               기  호            문자열\n
604  ===========               ======            ======\n
605  날짜-시간 형식 문자열     D_T_FMT           "%s"\n
606  날짜 형식 문자열          D_FMT             "%s"\n
607  시간 형식 문자열          T_FMT             "%s"\n
608  요일 명칭                 DAY_1             "%s"\n
609                            DAY_2             "%s"\n
610                            DAY_3             "%s"\n
611                            DAY_4             "%s"\n
612                            DAY_5             "%s"\n
613                            DAY_6             "%s"\n
614                            DAY_7             "%s"\n
615  요일의 약칭               ABDAY_1           "%s"\n
616                            ABDAY_2           "%s"\n
617                            ABDAY_3           "%s"\n
618                            ABDAY_4           "%s"\n
619                            ABDAY_5           "%s"\n
620                            ABDAY_6           "%s"\n
621                            ABDAY_7           "%s"\n
622  월 명칭                   MON_1             "%s"\n
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
634  월의 약칭                 ABMON_1           "%s"\n
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
646  AM 문자열                 AM_STR            "%s"\n
647  PM 문자열                 PM_STR            "%s"\n
648  년 기호                   YEAR_UNIT         "%s"\n
649  월 기호                   MON_UNIT          "%s"\n
650  일 기호                   DAY_UNIT          "%s"\n
651  시 기호                   HOUR_UNIT         "%s"\n
652  분 기호                   MIN_UNIT          "%s"\n
653  초 기호                   SEC_UNIT          "%s"\n
654  년도 포맷 문자열          ERA_FMT           "%s"\n
700 COMMENT: lc_monetary 포맷 문자열
701  화폐 맞춤(Custom)\n
702  =================\n
703  설       명                                     기  호               문자열\n
704  ===========                                     ======               ======\n
705  통화 기호                                       CRNCYSTR             "%s"\n
706  화폐 lconv 값\n
707  =============\n
708  설       명                                     lconv    요소          값 \n
709  ===========                                     =============        =====\n
710  국제 통화 기호                                  int_curr_symbol      "%s"\n
711  지역 통화 기호                                  currency_symbol      "%s"\n
712  화폐의 소숫점                                   mon_decimal_point    "%s"\n
713  화폐의 1천 단위 분리 기호                       mon_thousands_sep    "%s"\n
714  화폐의 숫자 그룹의 크기                         mon_grouping         "%s"\n
715  화폐의 + 부호                                   positive_sign        "%s"\n
716  화폐의 - 부호                                   negative_sign        "%s"\n
717  국제 소수 숫자의 수                             int_frac_digits      '%s'\n
718  소수 숫자의 수                                  frac_digits          '%s'\n
719  기호가 양의 값 앞/뒤에 있음                     p_cs_precedes        '%s'\n
720  기호와 + 값 사이의 공백                         p_sep_by_space       '%s'\n
721  기호가 음의 값 앞/뒤에 있음                     n_cs_precedes        '%s'\n
722  기호와 - 값 사이의 공백                         n_sep_by_space       '%s'\n
723  + 부호의 위치                                   p_sign_posn          '%s'\n
724  - 부호의 위치                                   n_sign_posn          '%s'\n
800 COMMENT: lc_numeric 포맷 문자열
801  숫자 맞춤(Custom)\n
802  =================\n
803  설       명               기  호            문자열\n
804  ===========               ======            ======\n
805  기수(radix) 문자          RADIXCHAR         "%s"\n
806  1천 단위 분리 기호        THOUSEP           "%s"\n
807  대체 숫자                 ALT_DIGIT         "%s"\n
808  숫자 lconv 값\n
809  =============\n
810  설       명               이름                값 \n
811  ===========               ====              =====\n
812  기수 문자                 decimal_point     "%s"\n
813  1천 단위 분리 기호        thousands_sep     "%s"\n
814  그룹화                    grouping          "%s"\n
900 COMMENT: 기타 포맷 문자열
901  기타 맞춤(Custom)\n
902  =================\n
903  설       명               기  호            문자열\n
904  ===========               ======            ======\n
905  yes 문자열                YESSTR            "%s"\n
906  no 문자열                 NOSTR             "%s"\n
907  지정                      DIRECTION         "%s"\n
1000 COMMENT: 코드 세트 포맷 문자열
1001  코드 세트 값\n
1002  ============\n
1003  chr     hex     oct     dec\n
1004  ===========================\n
1005  %1$3s%2$8x%2$8o%2$8d\n
1006    chr     hex     oct     dec\n
1007  ===============================\n
1008  %1$6s%2$7x%2$9o%2$8d\n
1100 COMMENT: list_lang 포맷 문자열
1101  설치된 장소\n
1102  ===========\n
1103  장   소           수정자\n
1104  =======           ======\n
1105  %-18s%s%s%s%s\n
1106  나라 이름에 대한 자세한 내용은 NLS Concepts and Tuturial을 참조하십시오.\n
1107  =======================================================================\n
1200 COMMENT: 포맷 문자열 lconv p_sign_posn 및 n_sign 값
1201  lconv p_sign_posn 및 n_sign 값\n
1202  ==============================\n
1203  %-6s는 괄호로 수량 및 통화_기호를 묶습니다.\n
1204  %-6s 부호는 수량 및 통화_기호 앞에 옵니다.\n
1205  %-6s 부호는 수량 및 통화_기호 뒤에 옵니다.\n
1206  %-6s 부호는 통화_기호 바로 앞에 있습니다.\n
1207  %-6s 부호는 통화_기호 바로 뒤에 있습니다.\n
1208  %-6s 정보는 현재 장소에서 사용할 수 없습니다.\n
1300 COMMENT: p_cs_precedes 및 n_cs_precedes 값에 대한 포맷 문자열
1301  lconv p_cs_precedes 및 n_cs_precedes 값\n
1302  =======================================\n
1303  %-6s 통화_기호는 수량 뒤에 있습니다.\n
1304  %-6s 통화_기호는 수량 앞에 있습니다.\n
1305  %-6s 정보는 현재 장소에서 사용할 수 없습니다.\n
1400 COMMENT: p_sep_by_space 및 n_sep_by_space에 대한 포맷 문자열
1401  lconv p_sep_by_space 및 n_sep_by_space 값\n
1402  =========================================\n
1403  %-6s currency_symbol과 quantity 사이가 띄어지지 않습니다.\n
1404  %-6s currency_symbol과 quantity 사이가 띄어집니다.\n
1405  %-6s 현재 장소에서 한 정보가 아닙니다.\n
1500 COMMENT: 그룹화 및 화폐_그룹화 포맷 문자열
1501  lconv 그룹화 및 화폐_그룹화 값\n
1502  ==============================\n
1503    %-6s는 더 이상 그룹화하지 않습니다.\n
1504    %-6s 이전 요소가 숫자의 나머지에 반복적으로 사용됩니다.\n
1505    %-6s는 현재 그룹 내의 숫자의 갯수입니다.\n
1506 기타
1600 COMMENT: iconv 코드 세트명 포맷 문자열
1601  설치된 코드 세트 변환\n
1602  =====================\n
1603  From명              To명\n
1604  ======              ====\n
1605  %-20s%-20s\n
1606  이름은 코드 세트명의 처음 4개의 문자와 최종 문자로 구성됩니다.\n
1607  ==============================================================\n
1700 COMMENT: ctype_customs 포맷 문자열
1701  Ctype 맞춤(Custom)\n
1702  ==================\n
1703  설       명               기  호            문자열\n
1704  ===========               ======            ======\n
1705  문자당 바이트             BYTES_CHAR        "%s"\n
1706  대체 구두점               ALT_PUNCT         "%s"\n
