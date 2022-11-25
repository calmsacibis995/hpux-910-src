$set 1
1 audisp: you do not have the appropriate privileges.\n
2 audisp: can't read audit file %s.\n
3 audisp: can't use both -p and -f options.\n
4 audisp [ -u username ] [ -e eventname ] [ -c syscall ]\n
5        [ -p ] [ -f ] [ -l ttyid ] [ -t mmddhhmm[yy] ]\n
6        [ -s mmddhhmm[yy] ] audit_filename ...\n
7 audisp: bad start time in -t option.\n
8 audisp: bad stop time in -s option.\n
9 audisp: start time is greater than stop time.\n
10 audisp: all specified users names are invalid.\n
11 audisp: all specified tty names are invalid.\n
12 audisp: all specified event names are invalid.\n
13 audisp: cannot change to /dev directory.\n
14 audisp: cannot open /dev directory.\n
15 \naudisp: bad audit record.\n
16 \naudisp: bad audit record body.\n
17 \naudisp: malloc failed.\n
18 \naudisp: bad aud_flag struct.\n
19 Note: # files opened successfully = %d.\n
20 audisp: cannot back-reference pid ident. information.\n
21 audisp: attempt to self-audit failed.\n
22 TIME              PID E  EVENT   PPID    AID   RUID   RGID   EUID   EGID TTY\n\n
23 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n
24      
25  
26 users and aids:\n
27 All users are selected.\n
28 Selected the following events:\n
29 All events are selected.\n
30 ttys and ttydevs:\n
31 All ttys are selected.\n
32 Selecting only successful events.\n
33 Selecting only failed events.\n
34 Selecting successful & failed events.\n
35 start time :\n
36 stop time :\n
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
61 no acl's recorded\n
62 uid=%lu, 
63 gid=%lu, 
64 mode=%#o
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
87 audisp: cannot handle so many audit files. (Some may be CDF elements.)\n
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
