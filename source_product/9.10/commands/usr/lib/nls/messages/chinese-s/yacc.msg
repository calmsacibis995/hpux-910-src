$set 1
$quote "
1	"找不到语法分析程序 !1"
4	"项目太大"
5	"\n!1/!2 终结, !3/!4 非终结\n"
6	"!1/!2 语法规则, !3/!4 状态\n"
7	"!1 移进/归约, !2 归约/归约冲突\n"
8	"!1/!2 使用了工作集\n"
9	"内存: 状态, 等等, !1/!2, 语法分析程序 !3/!4\n"
10	"!1/!2 不同的超前行集\n"
11	"!1 多余的闭合\n"
12	"!1移进项, !2 例外\n"
13	"!1 转移(goto) 项\n"
14	"!1 项由缺省转移(goto) 存入\n"
15	"\n冲突: "
16	"!1 移进/归约"
17	", "
18	"!1 归约/归约"
19	"\n"
20	" 重大错误: "
21	" 错误: "
22	"\"!1\", 行 !2:"
23	"\tNULL"
24	" { "
25	"!1 "
26	"}"
27	"非终结符 !1 未定义!"
$
$ FOR LOCALIZERS: pyeild is a subroutine name.
$
28	"内部 Yacc 错误: pyield !1"
29	"\n!1: "
30	" !1\n"
$
$ FOR LOCALIZERS: nolook is not a routine.  Think of this error as saying
$       "yacc has encountered an error because we are in a state and
$        need a lookahead token but no look ahead token exists."
$
31	"yacc 状态/无超前错误"
32	"状态太多, 试用选项 -Ns"
33	"putitem(!1), 状态 !2\n"
34	"超出状态空间, 试用选项 -Nm"
35	"非终结符 !1 绝不能导出任意标志串"
36	"!1: "
37	"!1 !2, "
38	"\n"
39	"工作集溢出, 试用选项 -Nm"
40	"\n状态 !1, 无超前 = !1\n"
41	"标志已设置!\n"
42	"\t!1"
43	"\n"
44	"超前设置太多, 试用选项 -Nl"
45	"不能打开 !1"
46	"不能打开 !1"
47	"在 yacc 中 `o' 标志现在为缺省值\n"
48	"Ratfor Yacc 失效: 报歉...\n"
49	"非法 -N 选项"
50	"指定的表太小: !1"
51	"非法 -X 选项"
52	"非法 -X 选项"
53	"语法分析程序重置为 !1\n"
54	"非法选项: !1"
56	"不能打开 !1"
57	"不能打开临时文件"
58	"不能打开输入文件"
59	"错误的 %start 结构"
60	"%type 中语法错误"
61	"标号 !1 类型重复说明"
62	"非终结符 !1 类型重复说明"
63	"!1 的优先级重复说明"
64	"优先级太多"
65	"!1 的类型重复说明"
66	"请提早定义 !1 的类型号"
67	"语法错误"
68	"在 %% 之前出现了意外的文件尾"
69	"第一规则语法错误"
70	"在语法规则 LHS 上标号非法"
71	"非法规则: 缺少分号或 | ?"
72	"非法的 %prec 格式"
73	"在 %prec 后非终结符 !1 非法"
74	"太多过程规则, 试用 -Np 选项"
75	"必须返回一个值, 因为 LHS 有类型"
76	"在处理过程操作中缺省操作引起潜在的类型冲突"
77	"太多过程规则, 试用 -Np 选项"
78	"太多非终结符, 试用选项 -Nn"
79	"太多终结符, 限制为 !1 个"
80	"非法转义"
81	"非法 \\nnn 结构"
82	"'\\000' 非法"
83	"在 id 和字符串中字符太多, 试用选项 -Nc"
84	"非终结 <...> 语句"
85	"类型太多"
86	"非法或缺少 ' 或 \""
87	"非法转义, 或非法保留字: !1"
88	"必须为 !1 指定类型"
89	"应提早定义 !1"
90	"在处理 %union 时遇到文件尾符"
91	"在 %} 之前出现文件尾符"
92	"非法注释"
93	"文件结尾在注释内"
94	"在 $<ident> 语句中语法错误"
95	"$!1 使用非法"
96	"必须指定 $!1 的类型"
97	"文件结尾在注释内"
98	"在字符串或字符常量中有新行。"
99	"文件结尾在字符串或字符常量中"
$set 2
$quote "
100	"操作未终止"
$
$ FOR LOCALIZERS
$ 105 \n!1: reduce/reduce conflict (red'ns !2 and !3 ) on !4
$
$ red'ns is shorthand for reductions.  A reduction is parser rule
$ by which a set of terminals and non-terminals on the parser stack
$ can be replaced by a single non-terminal symbol.  This is a common
$ error message and is issued when a user attempts to run an ambiguous
$ input specification through yacc.
$ Here is an trivial yacc specification that can cause this error:
$ %%
$ a:   "identical strings"
$    | "identical strings"
$    ;
$
105	"\n!1: 归约/归约冲突 (归约 !2 和 !3) 发生在 !4"
106	"操作表溢出"
107	"\t"
108	"!1 "
109	"\n"
110	"在操作表中没有空间"
$
$ FOR LOCALIZERS
$ 111 !1: gotos on
$
$ It is followed by one or more occurrences of message 25:
$  25  "!1 "
$ In this situation, each occurence of message 25 will be replaced with a
$ symbol name that represents some next-token value.
$
$ In parser terms, this is used to output the gotos for the nontermninals
$ in a production.  An example of what a completed output line using messages
$ 111 and 25 is:
$
$   current_state: gotos on $accept state1 state2 errorstate
$
$ current_state, state1, state2 and errorstate would be production names
$ that the user entered in his input specification.  This message is used
$ to explain low level actions on the part of the parser (it is not used much).
$
111	"!1: 转移至 "
112	"!1 "
113	"\n"
114	"\n!1: 移位/归约冲突(移位 !2, 未归约 !3) 发生在 !4"
115	"\n状态 !1\n"
116	"\t!1\n"
117	"\t!1\n"
118	"\n\t!1  "
119	"接受"
120	"错误"
121	"移位 !1"
122	"归约 !1"
123	"\n\t. 归约 !1\n\n"
124	"\n\t. 错误\n\n"
125	"\t!1  转移到 !2\n"
126	"规则没有归约:   !1\n"
127	"!1 规则绝不会归约\n"
129	"错误的临时文件"
130	"错误的临时文件"
131	"状态 !1: 空(null)\n"
132	"一个数组溢出, 试用选项 -Na"
133	"一个数组溢出, 试用选项 -Na"
135	"一个数组溢出, 试用选项 -Na"
137	"在优化一个数组时空间溢出, 试用选项 -Na"
$
$ FOR LOCALIZERS
$ 138 clobber of a array, pos'n !1, by !2,\n\t\tcheck for multiple
$     definition of token # !3
$
$ Clobber means destroyed.  In this case one element of an array is being
$ over-written by another.  !1, !2, and !3 are all integer values.
$
138	"数组破坏, !1 被 !2 覆盖, \n\t\t请检查标志 # !3 的多次定义"
140	"一个数组溢出, 试用选项 -Na"
141	"使用优化程序空间: 输入 !1/!2, 输出 !3/!4\n"
142	"!1 表项, !2 零\n"
143	"最大扩展: !1, 最大偏移: !2\n"
144	"没有空间, 试用选项 -Nm "
145	"没有空间 - 对于不能分配内部使用内存"
146	"-Nt 没有实现, 忽略"
147	"表定义符 -N!1 不认识(忽略)"
148	"在 -p 变量中缺少外部名前辍"
149	"没有语法文件名变量"
150	"在文件名之后有多余的变量或变量格式错误"
151	" 警告: "
152	"标志名 '!1' 中有非法字符"
153	"变量 -b 缺少文件前辍名"
