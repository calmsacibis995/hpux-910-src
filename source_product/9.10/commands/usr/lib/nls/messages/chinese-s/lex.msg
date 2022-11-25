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
1	"����"
2	"����"
3	"�к�"
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
1	"δ֪ѡ�� !1\n"
2	"����״̬ !1 !2\n"
$
$ L10N note: Leave '%e' as it is.  It is a source construct.
$            Lex has a number or source directives that allow users to specifiy
$            sizes for internal data structures.  %e is one of them.
$            This message is telling the user to add a line to in the input
$            specification similar to "%e 1000".
$
3	"�﷨�ṹ̫��, ���� %e ��\n"
4	"�﷨�ṹ̫��\n"
5	"���ܷ����ڴ� - calloc ����ֵΪ��\n"
$
$ L10N note: calloc is a unix system routine that is called to allocate memory.
$            I suggest you translate the message to the equivalent of
$               "Unable to allocate memory - calloc returns a 0"
$
6	"�����ַ�̫��\n"
7	"�ַ�ֵ !1 ������Χ\n"
8	"ch ����Ҫ����˵��\n"
9	"�ַ� !1 ʹ��������\n"
10	"�ַ� '!1' ʹ��������\n"
$
$ L10N note: Leave '%p' as it is.  It is a source construct.
$            See the comment of msg 3.
$
11	"λ��̫��, ���� %p ��\n"
12	"λ��̫��\n"
13	"û�н��������ִ�\n"
$
$ L10N note: Leave '%a' as it is.  It is a source construct.
$            See the comment of msg 3.
$
14	"������̫��, ���� %a ��\n"
15	"������̫��\n"
16	"�ļ�������־��ע����\n"
17	"�ļ�������־���ִ����ַ�������\n"
18	"һ��״̬��λ��̫�� - acompute\n"
$
$ L10N note: acompute is a subroutine name.
$
19	"ûΪ char �������ռ�\n"
20	"��������̫��, ���� -Xa ѡ��\n"
21	"�ַ���𳬹�����ֽ� !1\n"
22	"��ִ�����Ӧ������ %% ֮��\n"
23	"�Ƿ����� !1\n"
24	"����˵��̫��\n"
25	"�� !1: !2\n"
26	"����̫��\n"
27	"��ʼ����̫��, ���� -XS ѡ��\n"
28	"�ַ����̫��, ���� -Xc ѡ��\n"
29	"����̫��, ���� -Xd ѡ��\n"
30	"״̬̫��\n"
$
$ L10N note: Leave '%n' as it is.  It is a source construct.
$            See the comment of msg 3.
$
31	"���� %n ��\n"
32	"������Χ����Ϊ��\n"
33	"����̫С, ���ܿ�ʼ\n"
34	"����̫С, ��������״̬\n"
35	"����������װ�ĺ���̫С\n"
36	"�����﷨�ṹ�ĺ���̫С\n"
37	"δ�ҵ����� !1\n"
38	"���ܶ������ļ� !1\n"
39	"���ܶ���׼�����豸\n"
40	"����ִ����������ִ�ͷ\n"
41	"ȱ�� Lex ��������, �ļ� !1\n"
42	"����δ��ֹ\n"
43	"�򲻿� !1 \n"
44	"�򲻿��ļ� !1\n"
45	"������ֲ�ַ����\n"
46	"lex ��ǰ��֧�� ratfor\n"
47	"ȱ�ٶε��޽��\n"
48	"δ������ʼ���� !1\n"
49	"δ���������� - �趨�ִ�Ϊ��(NULL)\n"
$
$ L10N note: Leave '%o' as it is.  It is a source construct.
$            See the comment of msg 3.
$
50	"��������, ���� %o ��\n"
51	"�ļ��������\n"
52	"�ҵ���λ�ַ���\n"
53	"�ַ���̫��\n"
$
$ L10N note: Leave '%k' as it is.  It is a source construct.
$
54	"̫��ѹ���ַ����, ���� %k ��\n"
55	"ʹ����̫�����ʼ����, ���� -Xs ѡ��\n"
56	"������̫�����ʼ����, ���� -Xs ѡ��\n"
$ 57 is open for use.
58	"ȥ������� '/'\n"
$
$ Messages 59-62 end up concatenated on a single line.  They are output if
$ the user has requested a "summary" of the machine generated.  The -v
$ option is used to generate the output.  All optional arguments are
$ filled with integer values.
$
59	"!1/!2 �ڵ�(%e), !3/!4 λ��(%p), !5/!6 (%n), !7������, "
60	"!1/!2 ѹ���ַ����(%k), "
61	"!1/!2 ѹ��������(%a), "
62	"!1/!2 �����(%o)\n"
63	"�Ƿ���Χ [!1-!2], 'From' ֵ > 'To' ֵ\n"
64	"�Ƿ���Χ [!1!2-!3!4], 'From' ֵ > 'To' ֵ\n"
65	"ͬһ��Χ�ڵ�Ԫ�ر�������ͬһ�ַ���\n"
66	"��Χ�Ľ�β�ַ������С��ͬ\n"
67	"�Ƿ���������� [. .]\n"
68	"�Ƿ��ĵȼ���� ([= =])\n"
69	"�Ƿ����ַ�������ʽ ([: :])\n"
70	"�Ƿ� -X ѡ��(������)\n"
71	"ָ���ı�̫С(������)\n"
72	"�޷�ʶ��ı�˵�� -N!1 (������)\n"
73	"����������� !1 !2\n"
74	"�ҵ��յ�������� ([. .]), �����ԡ�\n"
75	"locale ˵��̫��\n"
$
$ L10N note: Leave '%l' as it is.  It is a source construct.
$ !1 is replaced with any invalid locale such as japanese or chinese-t.
$
76	"��֧�� locale '!1' �� %l, ��ȱʡ�� locale ����\n"
77	"^ ��������֧���� CCL ([ ]) �к��ж��ֽ��ַ�\n"
78	"������ [. .], [= =], �� [: :] ��϶��ֽ��ַ�\n"
79	"�﷨���� - ���ܱ���\n"
80	"yacc ջ���\n"
81	"�﷨����\n"
82	"����Ϊ yacc ջ����ռ�\n"