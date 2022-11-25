
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
1	"오류"
2	"경고"
3	"행"
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
1	"알 수 없는 선택 항목 !1\n"
2	"좋지 않은 상태 !1 !2\n"
$
$ L10N note: Leave '%e' as it is.  It is a source construct.
$            Lex has a number or source directives that allow users to specifiy
$            sizes for internal data structures.  %e is one of them.  
$            This message is telling the user to add a line to in the input 
$            specification similar to "%e 1000".
$
3	"Parse tree가 너무 큽니다, %e num을 사용하십시오.\n"
4	"Parse tree가 너무 큽니다.\n"
5	"이크 - calloc이 0을 리턴합니다.\n"
$
$ L10N note: calloc is a unix system routine that is called to allocate memory.
$            I suggest you translate the message to the equivalent of 
$               "Unable to allocate memory - calloc returns a 0"
$
6	"너무 많은 문자들이 쌓였습니다.\n"
7	"범위(range)를 벗어난 문자값 !1\n"
8	"ch 표은 다시 정의할 필요가 있습니다.\n"
9	"문자 !1이(가) 두번 사용되었습니다.\n"
10	"문자 '!1'이(가) 두번 사용되었습니다.\n"
$
$ L10N note: Leave '%p' as it is.  It is a source construct.
$            See the comment of msg 3.
$
11	"position이 너무 많습니다. %p num을 사용하십시오.\n"
12	"position이 너무 많습니다.\n"
13	"비 종료 문자열\n"
$
$ L10N note: Leave '%a' as it is.  It is a source construct.
$            See the comment of msg 3.
$
14	"transition이 너무 많습니다. %a num을 사용하십시오.\n"
15	"transition이 너무 많습니다.\n"
16	"주석(comment)안에 EOF가 있습니다.\n"
17	"문자열이나 문자 상수안에 EOF가 있습니다.\n"
18	"한 상태(state)에 position이 너무 많습니다 - acompute\n"
$
$ L10N note: acompute is a subroutine name.
$
19	"문자 표을 역으로 하기 위한 공간이 없습니다.\n"
20	"오른쪽 context가 너무 많습니다. -Xa 선택 항목을 사용하십시오.\n"
21	"문자 부류가 !1 바이트의 최대 크기를 초과합니다.\n"
22	"수행할 수 있는 문장은 %%의 오른쪽에 있어야 합니다.\n"
23	"잘못된 요청 !1\n"
24	"언어 지정자(specifier)가 너무 늦게 지정되었습니다.\n"
25	"행 !1: !2\n"
26	"정의가 너무 깁니다.\n"
27	"start condition이 너무 깁니다, -XS 선택 항목을 사용하십시오.\n"
28	"큰 문자 부류가 너무 많습니다, -Xc 선택 항목을 사용하십시오.\n"
29	"정의(definition)가 너무 많습니다, -Xd 선택 항목을 사용하십시오.\n"
30	"상태(state)가 너무 많습니다.\n"
$
$ L10N note: Leave '%n' as it is.  It is a source construct.
$            See the comment of msg 3.
$
31	"%n num을 사용하십시오.\n"
32	"반복 범위(iteration range)는 양수이어야 합니다.\n"
33	"core가 너무 작아 시작할 수 없습니다.\n"
34	"core가 너무 작아 state generation을 할 수 없습니다.\n"
35	"core가 너무 작아 final packing을 할 수 없습니다.\n"
36	"core가 너무 작아 parse tree를 구성할 수 없습니다.\n"
37	"정의 !1을(를) 찾을 수 없습니다.\n"
38	"입력 파일 !1을(를) 읽을 수 없습니다.\n"
39	"표준 입력(standard input)을 읽을 수 없습니다.\n"
40	"바꿀 문자열은 숫자로 시작할 수 없습니다.\n"
41	"Lex 드라이버가 없어짐, 파일 !1\n"
42	"수행(action)이 끝나지 않았습니다.\n"
43	"!1을(를) 열 수 없습니다.\n"
44	"파일 !1을(를) 열 수 없습니다.\n"
45	"이식할 수 없는 문자 부류\n"
46	"Ratfor는 현재 lex에서 지원되지 않습니다.\n"
47	"rules section delimiter 빠짐\n"
48	"시작 조건 !1이 정의되지 않았습니다.\n"
49	"translation이 주어지지 않았습니다 - 공백 문자열로 사용합니다\n"
$
$ L10N note: Leave '%o' as it is.  It is a source construct.
$            See the comment of msg 3.
$
50	"출력 표에 오버플로우가 생겼습니다. %o num을 사용하십시오.\n"
51	"premature eof\n"
52	"쉬프트(shift) 문자가 있습니다.\n"
53	"문자열이 너무 깁니다.\n"
$
$ L10N note: Leave '%k' as it is.  It is a source construct.
$
54	"packed character class가 너무 많습니다, %k num을 사용하십시오.\n"
55	"시작 조건이 너무 많이 사용되었습니다, -Xs 선택 항목을 사용하십시오.\n"
56	"시작 조건이 너무 많이 정의되었습니다, -Xs 선택 항목을 사용하십시오.\n"
$ 57 is open for use.
58	"불필요한 슬래쉬(slash)가 제거되었습니다.\n"
$
$ Messages 59-62 end up concatenated on a single line.  They are output if
$ the user has requested a "summary" of the machine generated.  The -v 
$ option is used to generate the output.  All optional arguments are
$ filled with integer values.
$
59	"!1/!2 nodes(%e), !3/!4 positions(%p), !5/!6 (%n), !7 transitions, "
60	"!1/!2 packed char classes(%k), "
61	"!1/!2 packed transitions(%a), "
62	"!1/!2 output slots(%o)\n"
63	"잘못된 범위 [!1-!2]. 'From' 값 > 'To' 값\n"
64	"잘못된 범위 [!1!2-!3!4]. 'From' 값 > 'To' 값\n"
65	"범위 안에 있는 요소(elements)들은 같은 문자 세트에 속해야 합니다.\n"
66	"범위는 그 끝에 같은 크기의 문자를 가져야 합니다.\n"
67	"잘못된 분류 기호 [. .]\n"
68	"잘못된 Equivalence Class ([= =])\n"
69	"잘못된 Character Class 수식 ([: :])\n"
70	"-X 선택 항목이 잘못되었습니다.(무시됩니다)\n"
71	"표 크기가 너무 작게 지정되었습니다.(무시됩니다)\n"
72	"알 수 없는 표 지정자 -N!1(무시됩니다)\n"
73	"잘못된 transition !1 !2\n"
74	"빈 분류 기호 ([. .])가 있습니다,(무시됩니다)\n"
75	"scale 지정자가 너무 늦게 지정되었습니다.\n"
$
$ L10N note: Leave '%l' as it is.  It is a source construct.
$ !1 is replaced with any invalid locale such as japanese or chinese-t.
$
76	"locale'!1'은 %l에 지원되지 않습니다, 기본 locale로 처리합니다.\n"
77	"^ 연산자는 다중 바이트 문자를 포함하는 CCL([])에 지원되지 않습니다.\n"
78	"다중 바이트 문자와 [. .], [= =], 또는 [: :]는 혼합할 수 없습니다.\n"
79	"구문 오류 - 보관(backup)할 수 없습니다.\n"
80	"yacc 스택 오버플로우\n"
81	"구문 오류\n"
82	"yacc 스택에 대한 공간을 할당할 수 없습니다.\n"
