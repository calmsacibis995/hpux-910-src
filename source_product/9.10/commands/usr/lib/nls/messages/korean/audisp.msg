$set 1
1 audisp: 필요한 권한이 없습니다.\n
2 audisp: 감사 파일 %s을(를) 읽을 수 없습니다.\n
3 audisp: 선택 항목 -p와 -f을(를) 사용할 수 없습니다.\n
4 audisp [ -u username ] [ -e eventname ] [ -c syscall ]\n
5        [ -p ] [ -f ] [ -l ttyid ] [ -t mmddhhmm[yy] ]\n
6        [ -s mmddhhmm[yy] ] audit_filename ...\n
7 audisp: 선택 항목 -t의 시작 시간이 잘못되었습니다.\n
8 audisp: 선택 항목 -s의 정지 시간이 잘못되었습니다.\n
9 audisp: 시작 시간이 정지 시간보다 느립니다.\n
10 audisp: 지정된 사용자 이름이 모두 잘못되었습니다.\n
11 audisp: 지정된 tty 이름이 모두 잘못되었습니다.\n
12 audisp: 지정된 event 이름이 모두 잘못되었습니다.\n
13 audisp: /dev 디렉토리로 변경할 수 없습니다.\n
14 audisp: /dev 디렉토리를 열 수 없습니다.\n
15 \naudisp: 감사 레코드가 잘못되었습니다.\n
16 \naudisp: 감사 레코드 body가 잘못되었습니다.\n
17 \naudisp: malloc하지 못했습니다.\n
18 \naudisp: aud_flag 구조가 잘못되었습니다..\n
19 Note: 열린 파일 수 = %d.\n
20 audisp: pid ident. information를 역으로 참조할 수 없습니다.\n
21 audisp: 자체 감사를 하지 못했습니다.\n
22 TIME              PID E  EVENT   PPID    AID   RUID   RGID   EUID   EGID TTY\n\n
23 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n
24      
25  
26 users and aids:\n
27 모든 사용자가 선택됩니다.\n
28 다음의 이벤트(event)가 선택됩니다:\n
29 모든 이벤트(event)가 선택됩니다.\n
30 tty와 ttydevs:\n
31 모든 tty가 선택됩니다.\n
32 잘된 이벤트(event)만을 선택합니다.\n
33 안된 이벤트(event)만을 선택합니다.\n
34 잘된 이벤트(event)와 안된 이벤트를 선택합니다.\n
35 시작 시간 :\n
36 정지 시간 :\n
37 F 
38 S 
39 SELF-AUDITING TEXT: 
40 RETURN_VALUE 1 = %d; 
41 RETURN_VALUE 2 = %d
42 PARAM #%1d (int) = %ld\n
43 PARAM #%1d (unsigned long) = %lu\n
44 PARAM #%1d (long) = %ld\n
45 PARAM #%1d (char string) = 
46 PARAM #%1d (int) = %lu\n
47 PARAM #%1d (int array) = 
49 %d (addr family); %s\n
50 PARAM #%1d (file path) = %lu (cnode);
51                        0x%08.8x (dev);
52                        %lu (inode);
53          (path) = 
54 PARAM #%1d (file desc) = %lu (idev);\n
55                        %lu (inum)\n
56 PARAM #%1d (time struct) = %lu (seconds); %lu (micro sec)\n
57 PARAM #%1d (timezone) = %d (min west); %d (dst time)\n
58 PARAM #%1d (addr of char) = %lu\n
59 PARAM #%1d (cast long ptr) = %lu\n
60 PARAM #%1d (acl's) = 
61 acl에 기록된 것이 없습니다.\n
62 uid=%lu, 
63 gid=%lu, 
64 모드=%#o
65 PARAM #%1d (file desc's) =\n
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
87 audisp: 많은 감사 파일들을 처리할 수 없습니다(어떤 것은 CDF요소일 수 있음)\n
88 [ 
89 이벤트=%s; 
90 사용자=%s; 
91 Real Grp=%s; 
92 Eff.Grp=%s; 
93 Origin=%s; 
94  ]
95 /dev/pty
96 /dev/pty
97 /dev/pty
98 /dev/pty
