$set 1
1 audisp: ´gÁö¼Ôè«ËÂÌßÀ¹¥é.\n
2 audisp: ÆàÂºãÁ§õ£èÃâ¿ú½ó %s.\n
3 audisp: ÆàÂº¨¸»ç¢ìÊõ-p ¨î-f è´¹ùîè.\n
4 audisp [ -u username ] [ -e eventname ] [ -c syscall ]\n
5        [ -p ] [ -f ] [ -l ttyid ] [ -t mmddhhmm[yy] ]\n
6        [ -s mmddhhmm [yy]] audit_filename...\n
7 audisp: -t è´¹ùîèÌßäù­â»çëğêéâ¼.\n
8 audisp: -s è´¹ùîèÌß£äÀÛ»çëğêéâ¼.\n
9 audisp: äù­â»çëğÁ±£äÀÛ»çëğ»ì.\n
10 audisp: ·©¼Ô¸«¯ÆÌß¢ìÊõÖı¨¹ĞÓèôÆàºÓ.\n
11 audisp: ·©¼Ô¸«¯ÆÌß tty ¨¹ĞÓèôÆàºÓ.\n
12 audisp: ·©¼Ô¸«¯ÆÌß¡Ú¢¶¨¹ĞÓèôÆàºÓ.\n
13 audisp: ÆàÂººÎãÄÍ·êİ¥ì /dev Í·êİ.\n
14 audisp: ÆàÂºëë©Í /dev Í·êİ.\n
15 \naudisp: êéâ¼Ìß£èÃâáÕêİ.\n
16 \naudisp: êéâ¼Ìß£èÃâáÕêİ¡ÀñÄ.\n
17 \naudisp: «êßÜ malloc ­©ºÛ.\n
18 \naudisp: êéâ¼Ìß aud_flag Ôµ¿º.\n
19 ÂÄµß: # ¿ú½óëë©Í¶ï¦À = %d.\n
20 audisp: ÆàÂºª÷ï¢§ìÖû pid ident. ä¾áÎ.\n
21 audisp: áü«±Øü¶ğ£èÃâ­©ºÛ.\n
22 TIME              PID E  EVENT   PPID    AID   RUID   RGID   EUID   EGID TTY\n\n
23 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n
24 
25 
26 ¢ìÊõÖı¨îæÑ¦Ã: \n
27 ·©¼Ô¢ìÊõÖıèôà¥è´§õ.\n
28 è´¥Ô¡¨¥â¡Ú¢¶: \n
29 ·©¼Ô¡Ú¢¶èôà¥è´§õ.\n
30 ttys ¨î ttydevs: \n
31 ·©¼Ô ttys èôà¥è´§õ.\n
32 ¨¥è´§õ¶ï¦ÀÌß¡Ú¢¶.\n
33 ¨¥è´§õ­©ºÛÌß¡Ú¢¶.\n
34 è´§õ¶ï¦À¨î­©ºÛÌß¡Ú¢¶.\n
35 äù­â»çëğ: \n
36 £äÀÛ»çëğ: \n
37 F
38 S
39 Øü¶ğ£èÃâ¼èºğ:
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
61 ¼æáÕêİ acl's\n
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
87 audisp: ÆàÂºİçÉÓçÛô¢¬ù£èÃâ¿ú½ó. (¼Ô¡ä¨«Ø¥»á CDF ¤ØÓû.)\n
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
