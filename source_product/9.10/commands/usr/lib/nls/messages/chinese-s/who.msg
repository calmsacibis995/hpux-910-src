$set 1
4 空槽\n
5 名字       行           时间          空闲    进程号 注释\n
6 名字       行           时间\n
7 id=%4.4s
8 term=%-3d  exit=%-3d
9 \n#  users=%d\n
10 \n用法: \twho [-rbtpludAasHTqcRm] [am i] [utmp_like_file]\n
11 \n用法: \twho [-rbtpludAasHTqRm] [am i] [utmp_like_file]\n
12 \nr\t运行层次\nb\t启动时间\nt\t改变时间\n
13 p\t进程而非 getty 或 users\n
14 l\t登录进程\nu\t有用信息\n
15 d\t死进程\nA\t统计信息\n
16 a\t全部(rbtpludA 选项)\n
17 s\twho 的简单格式（没有自上次输出或进程号后的时间）\n
18 H\t打印标题\n
19 T\ttty 状态（+ 可写, - 不可写, x 排它打开, ? 挂起）\n
20 q\t快速 who\n
21 c\t簇宽 who\n
22 %b %2d %H:%M
23   旧的
24   ．
25  %2u: %2.2u
26 R\t打印主机名\n
