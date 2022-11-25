$set 1
1 audisp: 您没有相应的特权。\n
2 audisp: 不能阅读审计文件%s。\n
3 audisp: 不能同时使用 -p 和 -f 选项。\n
4 audisp [ -u username ] [ -e eventname ] [ -c syscall ]\n
5        [ -p ] [ -f ] [ -l ttyid ] [ -t mmddhhmm[yy] ]\n
6        [ -s mmddhhmm[yy] ] audit_filename ...\n
7 audisp: 选项 -t 中开始时间有误。\n
8 audisp: 选项 -s 中终止时间有误。\n
9 audisp: 开始时间比终止时间晚\n
10 audisp: 所有指定的用户名都非法。\n
11 audisp: 所有指定的 tty 名都非法。\n
12 audisp: 所有指定的事件名都非法。\n
13 audisp: 不能改变到 /dev 目录。\n
14 audisp: 打不开 /dev 目录。\n
15 \naudisp: 审计记录非法。\n
16 \naudisp: 审计记录非法。\n 
17 \naudisp: malloc 失败。\n
18 \naudisp: aud_flag结构非法。\n
19 注意: # 成功打开文件 = %d。\n 
20 audisp: 不能返回参照 pid 标志信息。\n
21 audisp: 试图自审失败。\n
22 TIME              PID E  EVENT   PPID    AID   RUID   RGID   EUID   EGID TTY\n\n
23 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n
24  
25  
26 用户和设备:\n
27 选择所有的用户。\n
28 选择下列事件:\n
29 选择所有的事件。\n
30 ttys 和 ttydevs:\n
31 选择所有的 tty。\n
32 仅选择成功的事件。\n
33 仅选择失败的事件。\n
34 选择成功和失败的事件。\n
35 开始时间:\n
36 终止时间:\n
37 F
38 S
39 SELF-AUDITING TEXT: 
40 返回值 1 = %d; 
41 返回值 2 = %d
42 参数 #%1d (int)=%ld\n
43 参数 #%1d (unsigned long) = %lu\n
44 参数 #%1d (long)=%ld\n
45 参数 #%1d (char string) = 
46 参数 #%1d (int)=%lu\n
47 参数 #%1d (int array) = 
49 %d (addr family); %s\n
50 参数 #%1d (file path) = %lu (cnode);
51                        0x%08.8x (dev);
52                        %lu (inode);
53          (path) =
54 参数 #%1d (file desc) = %lu (idev);\n
55                        %lu (inum)\n
56 参数 #%1d (time struct) = %lu (seconds); %lu (micro sec)\n
57 参数 #%1d (timezone) = %d (min west); %d (dst time)\n
58 参数 #%1d (addr of char) = %lu\n
59 参数 #%1d (cast long ptr) = %lu\n
60 参数 #%1d (acl's) =
61 没有 acl 记录\n
62 uid=%lu, 
63 gid=%lu, 
64 mode=%#o
65 参数 #%1d (file desc's) =\n
66 fd#%1d:   %lu (idev); %lu (inum)\n
67 Stream Header {\n
68    pid = %d\n
69    ppid = %d\n
70    aid = %ld\n
71    uid = %d\n
72    gid = %d\n
73    euid = %d\n
74    egid = %d\n
75    tty = %ld\n
76 Stream Body =\n
77 /dev
78 /dev
79 syscon
80 systty
81 /dev
82 /dev
83 syscon
84 systty
85 ?????
86 %02d%02d%02d %02d:%02d:%02d
87 audisp: 不能处理如此多审计文件。(某些或许是 CDF 元素。)\n
88 [ 
89 Event=%s; 
90 User=%s; 
91 Real Grp=%s; 
92 Eff.Grp=%s; 
93 Origin=%s; 
94  ]
95 /dev/pty
96 /dev/pty
97 /dev/pty
98 /dev/pty
