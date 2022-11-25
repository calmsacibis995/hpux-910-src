$set 1
1 fbackup(1001): 超出虚拟内存\n
2 fbackup(1002): 备份没有完成: 读者或写者进程退出\n
3 fbackup(1003): 超出虚拟内存\n
4 fbackup(1004): 会话开始于 %s\n
5 fbackup(1005): 运行时间: %d 秒\n
6 fbackup(1006): 指定的读者太多\n
7 fbackup(1007): 指定的内存记录太多\n
8 fbackup(1008): 不能获得当前工作目录\n
9 fbackup(1009): 不能获得指定的共享内存段\n
10 fbackup(1010): 用这信号标志的 semget 失败\n
11 fbackup(1011): 对共享内存段 shmat 失败\n
12 fbackup(1012): 成功地重启动这个会话\n
13 fbackup(1013): 不能重启动这个会话\n
14 fbackup(1014): 读者 %d 创建子进程失败\n
15 fbackup(1015): 在读者 execl 后到达\n
16 fbackup(1016): semctl 失败\n
17 fbackup(1017): 为写者创建子进程失败\n
18 fbackup(1018): 在写者 execl 后到达\n
19 fbackup(1019): 警告: 指定文件中没有需要备份的\n
20 fbackup(1020): 意外退出\n
21 fbackup(1021): 恢复文件表\n
22 fbackup(1022): 不能恢复文件表\n
23 fbackup(1023): 您想以后重启动这个会话吗?\n
24 fbackup(1024): 这个会话的状态已被存起来\n
25 fbackup(1025): 不存这个会话的状态\n
26 fbackup(1026): 未存这个会话的状态,退出\n
27 fbackup(1027): 备份没有完成: 读者或写者进程退出\n
28 fbackup(1028): 收到一个 SIGTERM; 清除并退出\n
29 fbackup(1029): 意外退出\n
101 fbackup(1101): 意外地用完了备份\n
102 fbackup(1102): 不能 stat 文件 %s\n
103 fbackup(1103): 打不开文件 %s\n
104 fbackup(1104): 超出虚拟存储区\n
105 fbackup(1105): 打不开目录 %s\n
106 fbackup(1106): 超出虚拟存储区\n
107 fbackup(1107): unable to get file control information for file %s\n
108 fbackup(1108): file %s has been locked,\n and will not be backed up\n
201 fbackup(1201): 不能重设置文件 %s 的存取和修改时间\n
202 fbackup(1202): 在 ACL 中为文件 %s 指定了未知的用户或组\n; 可选记录没有备份。\n
301 fbackup(1301): 从文件 %d 开始继续\n
401 fbackup(1401): 多余变量: %s 被忽略\n
402 fbackup(1402): 请不要指定多于一个的 fbackup 级\n
403 fbackup(1403): 必须指定至少一个输出文件\n
404 fbackup(1404): 没有指定文件; (用至少一个 -i 和/或 -g 选项)\n
405 fbackup(1405): 选项 [0-9ieg] 中没有可与 -R 一起使用的选项\n
406 用法: /etc/fbackup -f device [-f device] ... [-0-9] [-nsuvyH]\n
407 \t[-i path] ... [-e path] ... [-g graph] ... [-d path] [-I path] [-V path] [-c config]\n
408 \n用法: /etc/fbackup -f device [-f device] ... -R restart [-nsuvyH]\n
409 \t[-d path] [-I path] [-V path] [-c config]\n\n
410 fbackup(1410): 如果选用 -u, 您必须选用 -g 一次\n
411 \t且不能选用 -i 或 -e\n
412 fbackup(1412): 打不开文件树(graph)文件 %s\n
413 fbackup(1413): 打不开配置文件 %s\n
414 fbackup(1414): 非法的配置文件标签 (%s) 被忽略\n
415 fbackup(1415): 配置文件错误在 %d 行\n
416 错误行: \n%s\n
417 fbackup(1417): 打不开日期文件 %s\n
418 fbackup(1418): 上一次第 %d 级的, 对于 %s 的文件树(graph)的备份会话\n
419 \t开始 : %s\n
420 \t完成 : %s\n
421 fbackup(1421): 没有对文件树(graph)文件 %s 的记载
422 (低于 %d 级)\n
423 fbackup(1423): 不能写入日期文件 %s\n
424 fbackup(1424): 打不开临时日期文件 %s\n
425        起始: %s
426        结束: %s\n
427 fbackup(1427): 在文件树(graph)文件 %s 中有非法记录\n
428 fbackup(1428): 在扩充的输出文件模式有错\n
429 fbackup(1429): 输出文件的个数不能超过 %d\n
501 fbackup(1501): 不能在函数搜索中 stat %s\n
502 fbackup(1502): 不能在函数搜索中 chdir 到 %s\n
503 fbackup(1503): 不能 chdir 到起始目录 %s\n
504 fbackup(1504): 打不开目录 `.' (%s)\n
505 fbackup(1505): 超出虚拟内存\n
506 fbackup(1506): 不能 stat 文件 %s\n
507 fbackup(1507): 不能 chdir 到 %s\n
508 fbackup(1508): 在 expanddir() 中, 不能重新调整 "cp2"。\n
509 fbackup(1509): findpath 初始化错误\n
510 fbackup(1510): 不能 stat %s\n
512 fbackup(1512): 超出虚拟内存\n
513 fbackup(1513): 在 dotdotnotparent 中 statcall 失败\n
514 fbackup(1514): 不能 stat %s\n
515 fbackup(1515): 不能 stat %s\n
516 fbackup(1516): %s 没有备份 - 'n' 选项 (NFS) 未指定\n
517 fbackup(1517): %s 没有备份 - 'n' 选项 (NFS) 未指定\n
601 fbackup(1601): 不能存取 -i/-e 文件 %s\n
602 fbackup(1602): 超出虚拟内存\n
603 fbackup(1603): 超出虚拟内存\n
604 fbackup(1604): 超出虚拟内存\n
605 fbackup(1605): 超出虚拟内存\n
701 <连接>
702 fbackup(1700): 超出虚拟内存\n
704 fbackup(1704): 超出虚拟内存\n
705 fbackup(1705): 不能在行索引文件 %s 上创建\n
706 键入重启动文件名(按 [回车] 则退出)\n
707 fbackup(1707): 不能创建重启动文件 %s 用于写\n
708 fbackup(1708): 打不开重启动文件 %s 用于读\n
709 fbackup(1709): 超出虚拟内存\n
710 fbackup(1710): 超出虚拟内存\n
712 fbackup(1712): 重启动会话开始于: %s\n
713 fbackup(1713): 文件树(graph)文件 %s 在 %d 级\n
714 fbackup(1714): 文件号 %d 处恢复(在卷号 %d)\n
715 fbackup(1715): 不能生成卷头栏文件 %s\n
801 fbackup(1801): 超出虚拟内存\n
802 fbackup(1802): 超出虚拟内存\n
$set 2
1 fabckup(2001): 读者 %d: 对内存段 shmat 失败\n
2 fabckup(2002): 读者 %d: 打不开文件 %s\n
3 fabckup(2003): 读者 %d: 不能 lseek 文件 %s\n
4 fabckup(2004): 读者 %d: 第一次读错误, 
5 fabckup(2005): 读者 %d: 第二次读错误, 
6 fabckup(2006): 读者 %d: 收到一个不用处理的信号\n
7 fabckup(2007): 读者 %d: 异常退出\n
8 fbackup(2008): reader %d: unable to read file %s,\n  file has been locked\n
$set 3
1 fabckup(3001): 写者对内存段 shmat 失败;错误号: %d\n
2 fabckup(3002): 超出虚拟内存\n
3 fabckup(3003): 正常 EOT\n
4 fabckup(3004): 写者异常退出\n
5 fabckup(3005): 警告: 文件号 %d 没有被备份\n
6 fabckup(3006): 写者丢失同步, fno=%d, filenum=%d\n
7 fabckup(3007): 警告: 文件号 %d (%s)\n
8 \t在 %d 尝试时处于激活状态\n
9 fabckup(3009): 警告: 文件号 %d (%s)\n
10 \t备份没有成功\n
11 fabckup(3011): 当写一个检查点 EOF 时, 出现写错误, 
12 fabckup(3012): 当写一个检查点记录时, 出现写错误, 
13 fabckup(3013): 当写数据记录时, 出现写错误, 
14 fabckup(3014): 写错误: 不能写到输出文件\n
15 fbackup(3015): 不支持块输出设备\n
16 fbackup(3016): 当卷 %d 在 %s 上准备好, 请按 [回车] 键?\n
17 fbackup(3017): 磁带不在带头, 倒带\n
18 fbackup(3018): 输出文件 %s 是非法类型\n
19 fbackup(3019): 您想输入一个新文件?\n
20 fbackup(3020): 请输入新文件名:\n
21 fbackup(3021): 超出虚拟内存\n
22 fbackup(3022): 您想继续该会话?\n
23 fbackup(3023): 请确认卷 %d 在 %s 中已准备好\n
24 fbackup(3024): 写卷 %d 到输出文件 %s\n
25 fbackup(3025): 在卷头写入出错\n
26 fbackup(3026): ioctl 错误, 不能查询输出文件\n
27 fbackup(3027): ioctl 错误, 不能查询输出文件\n
28 fbackup(3028): 当写一个检查点 EOF 时, 出现写错误,
29 fbackup(3029): 当写一个检查点记录时, 出现写错误,
30 fbackup(3030): 当写数据记录时, 出现写错误,
32 fbackup(3032): 打不开输出文件 %s\n
33 fbackup(3033): 输出文件类型非法; 请指定一个
34 不倒带的 UCB 设备,\n\t或自动倒带的 AT&T 
35 设备\n
36 fbackup(3036): 介质 %s 不能置于开始位置\n
37 fbackup(3037): 打不开输出文件 %s\n
43 fbackup(3043): 当写数据记录时, 出现写错误,
44 fbackup(3044): ioctl 错误, 不能查询输出文件\n
45 fbackup(3045): 当写数据记录时, 出现写错误,
46 fbackup(3046): 当卷 %d 在 %s 上准备好, 请按 [回车] 键?\n
47 fbackup(3047): 打不开输出文件 %s\n
48 fbackup(3048): 介质 %s 不能置于开始位置\n
49 fbackup(3049): 当卷 %d 在 %s 上准备好, 请按 [回车] 键?\n
50 fbackup(3050): 打不开输出文件 %s\n
51 fbackup(3051): 当卷 %d 在 %s 上准备好, 请按 [回车] 键?\n
52 fbackup(3052): 打不开输出文件 %s\n
54 fbackup(3054): 打不开输出文件 %s\n
101 在介质记录 %d\n
102 fabckup(3102): 试图恢复该卷\n
103 fabckup(3103): 从头开始重写该卷?\n
104 fabckup(3104): 写 2 个 EOF 并倒带\n
105 fabckup(3105): 写 2 个 EOF 并倒带\n
106 fbackup(3106): 请安装一个好磁带\n
107 fbackup(3107): 在 %s 中写 EOF 出错\n
108 fbackup(3108): 在 %s 中写记录时出错\n
109 fbackup(3109): 在 %s 中写 EOF 出错\n
110 fbackup(3110): 在 %s 中写记录时出错\n
111 fbackup(3111): 在 %s 中写记录时出错\n
112 fbackup(3112): 在 %s 中写 EOF 出错\n
113 fbackup(3113): 在 %s 中写记录时出错\n
114 fbackup(3114): 在 %s 中写 EOF 出错\n
115 fbackup(3115): 在 %s 中写记录时出错\n
116 fbackup(3116): 在 %s 中写记录时出错\n
117 fbackup(3117): 在 %s 中写记录时出错\n
118 fbackup(3118): 在 %s 中写记录时出错\n
119 fbackup(3119): 在 %s 中写记录时出错\n
120 fbackup(3120): 在 %s 中写记录时出错\n
121 fbackup(3121): 在 %s 中写记录时出错\n
122 fbackup(3122): 在 %s 中写记录时出错\n
123 fbackup(3123): 不能读以前设置的检查记录\n
124 fbackup(3124): 在 %s 中写记录时出错\n
125 fbackup(3125): 在 %s 中写 FSM 时出错\n
126 fbackup(3126): 在 %s 中写记录时出错\n
127 fbackup(3127): 在 %s 中写记录时出错\n
128 fbackup(3128): 在 %s 中写 EOF 出错\n
129 fbackup(3129): 在 %s 中写记录时出错\n
130 fbackup(3130): 在 %s 中写记录时出错\n
202 fbackup(3202): 这是这次会话的第 %d 卷!\n\t拒绝该卷\n
203 fbackup(3203): 卷 %d 已经用了 %d 次\n
204 fabckup(3204): 用的卷大于最大值, 您还想用这个卷吗?\n
205 fabckup(3205): 不能读卷头\n
301 fbackup(3301): 打不开输出文件 %s\n
303 fbackup(3303): ioctl 错误, 不能查询输出文件\n
304 fbackup(3304): 磁带操作错误\n
305 fbackup(3305): 打不开输出文件 %s\n
306 fbackup(3306): 这是这次会话的第 %d 卷!\n\t拒绝该卷\n
307 fbackup(3307): 卷 %d 已经用 %d 次 (最多: %d)\n
308 fbackup(3308): 您还想用这个卷吗"\n
309 fbackup(3309): 不能读卷头\n
310 fbackup(3310): enter '%s' when volume %d is ready on %s,\n or '%s' to discontinue:\n
$set 4
1 fabckup(4001): 自动 '%s'\n
2 fabckup(4002): 请用 '%s' 或 '%s' 回答\n
3 fabckup(4003): 信号操作失败: op=%d\n
4 fabckup(4004): 函数 mymalloc 不能分配空间\n
