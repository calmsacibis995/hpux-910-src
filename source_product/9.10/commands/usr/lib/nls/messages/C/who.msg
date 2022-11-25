$set 1
4 Empty slot.\n
5 NAME       LINE         TIME          IDLE    PID  COMMENTS\n
6 NAME       LINE         TIME\n
7   id=%4.4s
8  term=%-3d exit=%-3d
9 \n# users=%d\n
10 \nUsage:\twho [-rbtpludAasHTqcRm] [am i] [utmp_like_file]\n
11 \nUsage:\twho [-rbtpludAasHTqRm] [am i] [utmp_like_file]\n
12 \nr\trun level\nb\tboot time\nt\ttime changes\n
13 p\tprocesses other than getty or users\n
14 l\tlogin processes\nu\tuseful information\n
15 d\tdead processes\nA\taccounting information\n
16 a\tall (rbtpludA options)\n
17 s\tshort form of who (no time since last output or pid)\n
18 H\tprint header\n
19 T\tstatus of tty (+ writable, - not writable, x exclusive open, ? hung)\n
20 q\tquick who\n
21 c\tcluster-wide who\n
22 %b %2d %H:%M
23   old 
24    .  
25  %2u:%2.2u
26 R\tprint host name\n
