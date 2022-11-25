$set 1
1 audisp: 実行するための特権がありません。\n
2 audisp: 監査ファイル %s が読めません。\n
3 audisp: -p と -f オプションは一緒には使えません。\n
4 audisp [ -u username ] [ -e eventname ] [ -c syscall ]\n
5        [ -p ] [ -f ] [ -l ttyid ] [ -t mmddhhmm[yy] ]\n
6        [ -s mmddhhmm[yy] ] audit_filename ...\n
7 audisp: -t オプションの開始時間が正しくありません。\n
8 audisp: -s オプションの終了時間が正しくありません。\n
9 audisp: 開始時間は終了時間より大きくなっています。\n
10 audisp: 指定したすべてのユーザ名が正しくありません。\n
11 audisp: 指定したすべての tty 名が正しくありません。\n
12 audisp: 指定したすべてのイベント名が正しくありません。\n
13 audisp: /dev ディレクトリへ移動できません。\n
14 audisp: /dev ディレクトリをオープンできません。\n
15 \naudisp: 監査レコードが正しくありません。\n
16 \naudisp: 監査レコード本体が正しくありません。\n
17 \naudisp: malloc ができません。\n
18 \naudisp: aud_flag 構造体が正しくありません。\n
19 注意: オープンできたファイルの数 = %d\n
20 audisp: pid の認定情報を逆参照できません。\n
21 audisp: 自分自身に対する監査はできません。\n
22 TIME              PID E  EVENT   PPID    AID   RUID   RGID   EUID   EGID TTY\n\n
23 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n
24      
25  
26 ユーザと aid:\n
27 すべてのユーザが選択されました。\n
28 以下のイベントが選択されました:\n
29 すべてのイベントが選択されました。\n
30 tty と ttydev:\n
31 すべての tty が選択されました。
32 成功したイベントのみを選択しました。\n
33 失敗したイベントのみを選択しました。\n
34 成功/失敗したイベントを選択しました。\n
35 開始時間 :\n
36 終了時間 :\n
37 F 
38 S 
39 SELF-AUDIT TEXT: 
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
61 acl レコードがありません。\n
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
87 audisp: 監査ファイルが多過ぎて処理できません (CDF の要素があります)。\n
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
