
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
1	"����"
2	"���"
3	"��"
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
1	"�� �� ���� ���� �׸� !1\n"
2	"���� ���� ���� !1 !2\n"
$
$ L10N note: Leave '%e' as it is.  It is a source construct.
$            Lex has a number or source directives that allow users to specifiy
$            sizes for internal data structures.  %e is one of them.  
$            This message is telling the user to add a line to in the input 
$            specification similar to "%e 1000".
$
3	"Parse tree�� �ʹ� Ů�ϴ�, %e num�� ����Ͻʽÿ�.\n"
4	"Parse tree�� �ʹ� Ů�ϴ�.\n"
5	"��ũ - calloc�� 0�� �����մϴ�.\n"
$
$ L10N note: calloc is a unix system routine that is called to allocate memory.
$            I suggest you translate the message to the equivalent of 
$               "Unable to allocate memory - calloc returns a 0"
$
6	"�ʹ� ���� ���ڵ��� �׿����ϴ�.\n"
7	"����(range)�� ��� ���ڰ� !1\n"
8	"ch ǥ�� �ٽ� ������ �ʿ䰡 �ֽ��ϴ�.\n"
9	"���� !1��(��) �ι� ���Ǿ����ϴ�.\n"
10	"���� '!1'��(��) �ι� ���Ǿ����ϴ�.\n"
$
$ L10N note: Leave '%p' as it is.  It is a source construct.
$            See the comment of msg 3.
$
11	"position�� �ʹ� �����ϴ�. %p num�� ����Ͻʽÿ�.\n"
12	"position�� �ʹ� �����ϴ�.\n"
13	"�� ���� ���ڿ�\n"
$
$ L10N note: Leave '%a' as it is.  It is a source construct.
$            See the comment of msg 3.
$
14	"transition�� �ʹ� �����ϴ�. %a num�� ����Ͻʽÿ�.\n"
15	"transition�� �ʹ� �����ϴ�.\n"
16	"�ּ�(comment)�ȿ� EOF�� �ֽ��ϴ�.\n"
17	"���ڿ��̳� ���� ����ȿ� EOF�� �ֽ��ϴ�.\n"
18	"�� ����(state)�� position�� �ʹ� �����ϴ� - acompute\n"
$
$ L10N note: acompute is a subroutine name.
$
19	"���� ǥ�� ������ �ϱ� ���� ������ �����ϴ�.\n"
20	"������ context�� �ʹ� �����ϴ�. -Xa ���� �׸��� ����Ͻʽÿ�.\n"
21	"���� �η��� !1 ����Ʈ�� �ִ� ũ�⸦ �ʰ��մϴ�.\n"
22	"������ �� �ִ� ������ %%�� �����ʿ� �־�� �մϴ�.\n"
23	"�߸��� ��û !1\n"
24	"��� ������(specifier)�� �ʹ� �ʰ� �����Ǿ����ϴ�.\n"
25	"�� !1: !2\n"
26	"���ǰ� �ʹ� ��ϴ�.\n"
27	"start condition�� �ʹ� ��ϴ�, -XS ���� �׸��� ����Ͻʽÿ�.\n"
28	"ū ���� �η��� �ʹ� �����ϴ�, -Xc ���� �׸��� ����Ͻʽÿ�.\n"
29	"����(definition)�� �ʹ� �����ϴ�, -Xd ���� �׸��� ����Ͻʽÿ�.\n"
30	"����(state)�� �ʹ� �����ϴ�.\n"
$
$ L10N note: Leave '%n' as it is.  It is a source construct.
$            See the comment of msg 3.
$
31	"%n num�� ����Ͻʽÿ�.\n"
32	"�ݺ� ����(iteration range)�� ����̾�� �մϴ�.\n"
33	"core�� �ʹ� �۾� ������ �� �����ϴ�.\n"
34	"core�� �ʹ� �۾� state generation�� �� �� �����ϴ�.\n"
35	"core�� �ʹ� �۾� final packing�� �� �� �����ϴ�.\n"
36	"core�� �ʹ� �۾� parse tree�� ������ �� �����ϴ�.\n"
37	"���� !1��(��) ã�� �� �����ϴ�.\n"
38	"�Է� ���� !1��(��) ���� �� �����ϴ�.\n"
39	"ǥ�� �Է�(standard input)�� ���� �� �����ϴ�.\n"
40	"�ٲ� ���ڿ��� ���ڷ� ������ �� �����ϴ�.\n"
41	"Lex ����̹��� ������, ���� !1\n"
42	"����(action)�� ������ �ʾҽ��ϴ�.\n"
43	"!1��(��) �� �� �����ϴ�.\n"
44	"���� !1��(��) �� �� �����ϴ�.\n"
45	"�̽��� �� ���� ���� �η�\n"
46	"Ratfor�� ���� lex���� �������� �ʽ��ϴ�.\n"
47	"rules section delimiter ����\n"
48	"���� ���� !1�� ���ǵ��� �ʾҽ��ϴ�.\n"
49	"translation�� �־����� �ʾҽ��ϴ� - ���� ���ڿ��� ����մϴ�\n"
$
$ L10N note: Leave '%o' as it is.  It is a source construct.
$            See the comment of msg 3.
$
50	"��� ǥ�� �����÷ο찡 ������ϴ�. %o num�� ����Ͻʽÿ�.\n"
51	"premature eof\n"
52	"����Ʈ(shift) ���ڰ� �ֽ��ϴ�.\n"
53	"���ڿ��� �ʹ� ��ϴ�.\n"
$
$ L10N note: Leave '%k' as it is.  It is a source construct.
$
54	"packed character class�� �ʹ� �����ϴ�, %k num�� ����Ͻʽÿ�.\n"
55	"���� ������ �ʹ� ���� ���Ǿ����ϴ�, -Xs ���� �׸��� ����Ͻʽÿ�.\n"
56	"���� ������ �ʹ� ���� ���ǵǾ����ϴ�, -Xs ���� �׸��� ����Ͻʽÿ�.\n"
$ 57 is open for use.
58	"���ʿ��� ������(slash)�� ���ŵǾ����ϴ�.\n"
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
63	"�߸��� ���� [!1-!2]. 'From' �� > 'To' ��\n"
64	"�߸��� ���� [!1!2-!3!4]. 'From' �� > 'To' ��\n"
65	"���� �ȿ� �ִ� ���(elements)���� ���� ���� ��Ʈ�� ���ؾ� �մϴ�.\n"
66	"������ �� ���� ���� ũ���� ���ڸ� ������ �մϴ�.\n"
67	"�߸��� �з� ��ȣ [. .]\n"
68	"�߸��� Equivalence Class ([= =])\n"
69	"�߸��� Character Class ���� ([: :])\n"
70	"-X ���� �׸��� �߸��Ǿ����ϴ�.(���õ˴ϴ�)\n"
71	"ǥ ũ�Ⱑ �ʹ� �۰� �����Ǿ����ϴ�.(���õ˴ϴ�)\n"
72	"�� �� ���� ǥ ������ -N!1(���õ˴ϴ�)\n"
73	"�߸��� transition !1 !2\n"
74	"�� �з� ��ȣ ([. .])�� �ֽ��ϴ�,(���õ˴ϴ�)\n"
75	"scale �����ڰ� �ʹ� �ʰ� �����Ǿ����ϴ�.\n"
$
$ L10N note: Leave '%l' as it is.  It is a source construct.
$ !1 is replaced with any invalid locale such as japanese or chinese-t.
$
76	"locale'!1'�� %l�� �������� �ʽ��ϴ�, �⺻ locale�� ó���մϴ�.\n"
77	"^ �����ڴ� ���� ����Ʈ ���ڸ� �����ϴ� CCL([])�� �������� �ʽ��ϴ�.\n"
78	"���� ����Ʈ ���ڿ� [. .], [= =], �Ǵ� [: :]�� ȥ���� �� �����ϴ�.\n"
79	"���� ���� - ����(backup)�� �� �����ϴ�.\n"
80	"yacc ���� �����÷ο�\n"
81	"���� ����\n"
82	"yacc ���ÿ� ���� ������ �Ҵ��� �� �����ϴ�.\n"