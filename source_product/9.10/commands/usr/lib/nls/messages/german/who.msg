$set 1
4 Leerer Einschub.\n
5 NAME	     LINE	  TIME		IDLE	PID  KOMMENTARE\n
6 NAME	     LINE	  TIME\n
7   id=%4.4s
8  term=%-3d exit=%-3d
9 \n# Benutzer=%d\n
10 \nSyntax:\twho [-rbtpludAasHTqcRm] [am i] [utmp_like_Datei]\n
11 \nSyntax:\twho [-rbtpludAasHTqRm] [am i] [utmp_like_Datei]\n
12 \nr\trun-level\nb\tLadezeit \nt\tZeitÌnderungen\n
13 p\tAndere Prozesse als getty oder users\n
14 l\tlogin-Prozesse\nu\tsinnvolle Information\n
15 d\tdead-Prozesse\nA\tAbrechnungsinformation\n
16 a\talle (rbtpludA Optionen)\n
17 s\tKurzform von who (ohne Zeit seit letzter Ausgabe oder pid)\n
18 H\tDrucke Ûberschrift\n
19 T\tStatus von tty (+ beschreibbar, - nicht beschreibbar, x exclusiv offen, ? hÌngt)\n
20 q\tquick who\n
21 c\tcluster-weites who\n
22 %b %2d %H:%M
23   alt
24    .
25  %2u:%2.2u
26 R\tDrucke Hostname\n
