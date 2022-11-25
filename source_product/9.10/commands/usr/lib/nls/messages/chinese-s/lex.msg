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
1	"错误"
2	"警告"
3	"行号"
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
1	"未知选项 !1\n"
2	"错误状态 !1 !2\n"
$
$ L10N note: Leave '%e' as it is.  It is a source construct.
$            Lex has a number or source directives that allow users to specifiy
$            sizes for internal data structures.  %e is one of them.
$            This message is telling the user to add a line to in the input
$            specification similar to "%e 1000".
$
3	"语法结构太大, 试用 %e 数\n"
4	"语法结构太大\n"
5	"不能分配内存 - calloc 返回值为零\n"
$
$ L10N note: calloc is a unix system routine that is called to allocate memory.
$            I suggest you translate the message to the equivalent of
$               "Unable to allocate memory - calloc returns a 0"
$
6	"推入字符太多\n"
7	"字符值 !1 超出范围\n"
8	"ch 表需要重新说明\n"
9	"字符 !1 使用了两次\n"
10	"字符 '!1' 使用了两次\n"
$
$ L10N note: Leave '%p' as it is.  It is a source construct.
$            See the comment of msg 3.
$
11	"位置太多, 试用 %p 数\n"
12	"位置太多\n"
13	"没有结束符的字串\n"
$
$ L10N note: Leave '%a' as it is.  It is a source construct.
$            See the comment of msg 3.
$
14	"事务处理太多, 试用 %a 数\n"
15	"事务处理太多\n"
16	"文件结束标志在注释中\n"
17	"文件结束标志在字串或字符常量中\n"
18	"一个状态的位置太多 - acompute\n"
$
$ L10N note: acompute is a subroutine name.
$
19	"没为 char 表保留空间\n"
20	"右上下文太多, 试用 -Xa 选项\n"
21	"字符类别超过最大字节 !1\n"
22	"可执行语句应紧接在 %% 之后\n"
23	"非法请求 !1\n"
24	"语言说明太迟\n"
25	"行 !1: !2\n"
26	"定义太长\n"
27	"起始条件太长, 试用 -XS 选项\n"
28	"字符类别太大, 试用 -Xc 选项\n"
29	"定义太多, 试用 -Xd 选项\n"
30	"状态太多\n"
$
$ L10N note: Leave '%n' as it is.  It is a source construct.
$            See the comment of msg 3.
$
31	"试用 %n 数\n"
32	"迭代范围必须为正\n"
33	"核心太小, 不能开始\n"
34	"核心太小, 不能生成状态\n"
35	"用于最终组装的核心太小\n"
36	"用于语法结构的核心太小\n"
37	"未找到定义 !1\n"
38	"不能读输入文件 !1\n"
39	"不能读标准输入设备\n"
40	"替代字串不能用数字打头\n"
41	"缺少 Lex 驱动程序, 文件 !1\n"
42	"操作未终止\n"
43	"打不开 !1 \n"
44	"打不开文件 !1\n"
45	"不可移植字符类别\n"
46	"lex 当前不支持 ratfor\n"
47	"缺少段的限界符\n"
48	"未定义起始条件 !1\n"
49	"未给定事务处理 - 设定字串为空(NULL)\n"
$
$ L10N note: Leave '%o' as it is.  It is a source construct.
$            See the comment of msg 3.
$
50	"输出表溢出, 试用 %o 数\n"
51	"文件过早结束\n"
52	"找到移位字符。\n"
53	"字符串太长\n"
$
$ L10N note: Leave '%k' as it is.  It is a source construct.
$
54	"太多压缩字符类别, 试用 %k 数\n"
55	"使用了太多的起始条件, 试用 -Xs 选项\n"
56	"定义了太多的起始条件, 试用 -Xs 选项\n"
$ 57 is open for use.
58	"去除多余的 '/'\n"
$
$ Messages 59-62 end up concatenated on a single line.  They are output if
$ the user has requested a "summary" of the machine generated.  The -v
$ option is used to generate the output.  All optional arguments are
$ filled with integer values.
$
59	"!1/!2 节点(%e), !3/!4 位置(%p), !5/!6 (%n), !7事务处理, "
60	"!1/!2 压缩字符类别(%k), "
61	"!1/!2 压缩事务处理(%a), "
62	"!1/!2 输出槽(%o)\n"
63	"非法范围 [!1-!2], 'From' 值 > 'To' 值\n"
64	"非法范围 [!1!2-!3!4], 'From' 值 > 'To' 值\n"
65	"同一范围内的元素必须属于同一字符集\n"
66	"范围的结尾字符必须大小相同\n"
67	"非法的排序符号 [. .]\n"
68	"非法的等价类别 ([= =])\n"
69	"非法的字符类别表达式 ([: :])\n"
70	"非法 -X 选项(被忽略)\n"
71	"指定的表太小(被忽略)\n"
72	"无法识别的表说明 -N!1 (被忽略)\n"
73	"错误的事务处理 !1 !2\n"
74	"找到空的排序符号 ([. .]), 被忽略。\n"
75	"locale 说明太迟\n"
$
$ L10N note: Leave '%l' as it is.  It is a source construct.
$ !1 is replaced with any invalid locale such as japanese or chinese-t.
$
76	"不支持 locale '!1' 至 %l, 用缺省的 locale 处理\n"
77	"^ 操作符不支持在 CCL ([ ]) 中含有多字节字符\n"
78	"不能用 [. .], [= =], 或 [: :] 混合多字节字符\n"
79	"语法错误 - 不能备份\n"
80	"yacc 栈溢出\n"
81	"语法错误\n"
82	"不能为 yacc 栈分配空间\n"
