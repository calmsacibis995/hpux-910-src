$set 1
10 内部呼叫者形式 %s\n
11 mlock %s 失败\n
12 mlock %s 成功\n
13 打开超时\n
16 一般性的打开 (%d) 失败, 错误号 = %d\n
19 在 Dialers 文件中没有找到 %s\n
22 调用 gdial(%s)\n
23 dkcall(%s)\n
27 tcpdial 主机 %s, 
28 端口 %d\n
29 没有插座(socket): %s\n
31 没有插座(socket), 错误号%d\n
34 超时 tcpopen\n
37 系列: %d\n
38 端口: %d\n
39 地址: %08x\n
40 联接失败: %s\n
42 联接失败, 错误号 %d\n
45 unetdial 主机 %s
46 端口 %d\n
47 tcpopen 失败: 错误号 %d\n
48 打开失败 %s\n
50 COM1 返回 = %d\n
52 打开 sytek 超时\n
53 打开 2 失败 %s\n
54 mlock %s 失败\n
55 使用端口 %s
56 acu - %s, 
57 电话号码 %s\n
58 试 modem - %s, 
59 acu - %s, 
60 呼叫 %s: 
61 成功\n
62 失败\n
63 打不开 %s\n 
64 %s 已打开\n
65 打开 modem 超时\n
66 拨号失败, 子进程终止。\n
68 ACU 写错误 %d\n
69 拨号使用程序 %s\n
72 dcf 是 %d\n
73 Fork Stat %o\n
74 DN 写 %s\n 
75 超时
76 打不开 %s\n 
77 %s 已打开\n
78 打不开 %s\n 
79 dcf 是 %d\n
80 ACU 写错误 %d\n
81 ACU 写成功 %s\n
82 打开行 %s\n
83 失败
419 conn(%s)\n
420 getto ret %d\n
421 关闭呼叫者 (%d)\n
422 delock(%s)\n
423 呼叫失败: %s\n
424 需要 %s 型设备\n
425 所需设备没有找到\n
427 调用时间错误: %s\n
428 ProtoStr = %s\n
430 希望: (
431 获得\n
432 丢行错误号 - %d\n
435 已足够\n 
436 获得\n
437 超时\n
438 中断\n
439 EOT\n
440 sendthem (
441 延时\n
442 <NO CR>
443 <NO CR - MIDDLE IGNORED>\n
444 暂停\n
445 回送校验开\n
446 回送校验关\n
447 中断\n
448 ^M
449 )\n
524 fixline(%d,
525 %d)\n
527 fixline - direct\n
529 setline - %c\n
530 sytfixline - speek= %d\n
534 fixline(%d, 
535 %d)\n
641 onelock() 中 ulockf 失败\n
642 ulockf 文件 %s\n
643 文件已锁--不能读 (错误号 %d) -- 删除它!\n   
644 文件已锁--格式错误 -- 删除它!\n   
645 文件已锁--进程仍在执行 -- 不能删除它\n   
646 终止进程 (%d), 
647 返回 %d
648 -- 可以删去已锁的文件 (%s)\n
649 unlink() 中 ulockf 失败\n
650 ulockf 文件 %s\n
651 文件已锁--不能读 (错误号 %d) -- 删除它!\n   
652 -- 删去已锁的文件 (%s)\n
653 unlink() 中 ulockf 失败\n
657 连接(%s, 
658 %s)\n
1120 [-h] [-v] [-w n] [-x n] [-s speed] telno ...
1121 ct: 打不开 %s\n
1122 \t用法: %s %s\n
1123 (-w %s) 等待时间必须 > 0\n
1124 \t用法: %s %s\n
1125 \t 用法: %s %s\n
1126 没有指定电话号码!\n
1127 ct: 电话号码太长 -- %s\n
1128 ct: 电话号码非法 -- %s\n
1129 分配的拨号机波特率为 %s\n
1130 本系统无波特率为 %s 的拨号机\n
1131 %s 忙
1132 拨号机为 
1133 拨号机为 
1134  (%d minute(s))\n
1135  (%d) %s 拨号机 %s 忙\n
1136 是
1137 是
1138 等待 %d 分钟%s\n
1139 s
1140 您想等拨号机吗? ( y 代表 '是'):
1141 时间, 分钟?
1142 等拨号机\n
1143 *** 超时 ***\n
1144 挂机了? ( y 代表挂机):
1145 接通\n
1146 按 [回车] 键
1147 有个 uugetty: 退出\n
1148 启动 getty -h -t60 %s
1149 ct: 不能为 getty 创建子进程\n
1150 ct: 不能执行 getty\n
1151 \n再接吗?
1152 挂断(%d)\n
1153 挂断\n
1154 cleanup(%d)\n
1155 chmod/chown %s\n
1156 不能对 %s chmod/chown 
1157 对 %d\n
1158 ut_line %s, 
1159 ut_user %s, 
1160 ut_id %.4s, 
1161 ut_pid %d\n
1162 进程仍在执行\n
