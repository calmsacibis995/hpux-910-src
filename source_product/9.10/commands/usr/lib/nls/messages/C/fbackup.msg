$set 1
1 fbackup(1001): out of virtual memory\n
2 fbackup(1002): Backup did not complete : Reader or Writer process exit \n
3 fbackup(1003): out of virtual memory\n
4 fbackup(1004): session begins on %s\n
5 fbackup(1005): run time: %d seconds\n
6 fbackup(1006): too many readers specified\n
7 fbackup(1007): too many memory records specified\n
8 fbackup(1008): could not get the current working directory\n
9 fbackup(1009): cannnot get the specified shared memory segment\n
10 fbackup(1010): semget failed for the semaphore\n
11 fbackup(1011): shmat failed for the shared memory segment\n
12 fbackup(1012): successfully restarted the session\n
13 fbackup(1013): unable to restart the session\n
14 fbackup(1014): fork failed for reader %d\n
15 fbackup(1015): reached after reader execl\n
16 fbackup(1016): semctl failed\n
17 fbackup(1017): fork failed for writer\n
18 fbackup(1018): reached after writer execl\n
19 fbackup(1019): warning: none of the specified files needed to be backed up\n
20 fbackup(1020): Unexpected exit\n
21 fbackup(1021): recovered the file list\n
22 fbackup(1022): unable to recover the file list\n
23 fbackup(1023): will you restart this session later?\n
24 fbackup(1024): this session's state has been saved\n
25 fbackup(1025): did not save this session's state\n
26 fbackup(1026): exiting without saving this session's state\n
27 fbackup(1027): Backup did not complete : Reader or Writer process exit \n
28 fbackup(1028): received a SIGTERM; cleaning up and exiting\n
29 fbackup(1029): Unexpected exit\n
101 fbackup(1101): unexpectedly ran out of files to backup\n
102 fbackup(1102): unable to stat file %s\n
103 fbackup(1103): unable to open file %s\n
104 fbackup(1104): out of virtual memory\n
105 fbackup(1105): could not open directory %s\n
106 fbackup(1106): out of virtual memory\n
107 fbackup(1107): unable to get file control information for file %s\n
108 fbackup(1108): file %s has been locked,\n and will not be backed up\n
201 fbackup(1201): unable to reset access and modification times for file %s\n
202 fbackup(1202): unknown user or group specified in ACL for file: %s\n; optional entries are not backed up.\n
301 fbackup(1301): resuming at file %d\n
401 fbackup(1401): extra argument: %s ignored\n
402 fbackup(1402): please do not specify more than one fbackup level\n
403 fbackup(1403): at least one output file must be specified\n
404 fbackup(1404): no files have been specified; (use at least one -i and/or -g option)\n
405 fbackup(1405): none of the options [0-9ieg] may be used with -R\n
406 usage: /etc/fbackup -f device [-f device] ... [-0-9] [-nsuvyH]\n
407 \t[-i path] ... [-e path] ... [-g graph] ... [-d path] [-I path] [-V path] [-c config]\n
408 \nusage: /etc/fbackup -f device [-f device] ... -R restart [-nsuvyH]\n
409 \t[-d path] [-I path] [-V path] [-c config]\n\n
410 fbackup(1410): for the update (-u) to take place, exactly one graph (-g) file\n
411 \tand no include or exclude files (-i, -e) may be specified\n
412 fbackup(1412): unable to open graph file %s\n
413 fbackup(1413): unable to open configuration file %s\n
414 fbackup(1414): illegal configuration file label (%s) ignored\n
415 fbackup(1415): configuration file error at line %d\n
416 bad line:\n%s\n
417 fbackup(1417): cannot open the dates file %s for reading\n
418 fbackup(1418): the last level %d session for graph %s was\n
419 \tstarted  : %s\n
420 \tfinished : %s\n
421 fbackup(1421): no history is available for graph file %s 
422 (below level %d)\n
423 fbackup(1423): could not open the dates file %s for writing\n
424 fbackup(1424): unable to open the temporary dates file %s\n
425     STARTED: %s
426     ENDED: %s\n
427 fbackup(1427): illegal entry in graph file %s\n
428 fbackup(1428): error in expanding output file pattern\n
429 fbackup(1429): number of output files cannot be more than %d\n
501 fbackup(1501): could not stat %s in function search\n
502 fbackup(1502): could not chdir to %s in function search\n
503 fbackup(1503): could not chdir to starting directory %s\n
504 fbackup(1504): could not open directory `.' (%s)\n
505 fbackup(1505): out of virtual memory\n
506 fbackup(1506): could not stat file %s\n
507 fbackup(1507): could not chdir to %s\n
508 fbackup(1508): unable to readjust "cp2" in expanddir().\n
509 fbackup(1509): initialization error for findpath.\n
510 fbackup(1510): could not stat %s\n
512 fbackup(1512): out of virtual memory\n
513 fbackup(1513): statcall failed in dotdotnotparent\n
514 fbackup(1514): could not stat %s\n
515 fbackup(1515): could not stat %s\n
516 fbackup(1516): %s not backed up - 'n' option (NFS) not specified \n
517 fbackup(1517): %s not backed up - 'n' option (NFS) not specified \n
601 fbackup(1601): could not access -i/-e file %s\n
602 fbackup(1602): out of virtual memory\n
603 fbackup(1603): out of virtual memory\n
604 fbackup(1604): out of virtual memory\n
605 fbackup(1605): out of virtual memory\n
701  <LINK> 
702 fbackup(1702): out of virtual memory\n
704 fbackup(1704): out of virtual memory\n
705 fbackup(1705): unable to create on line index file %s\n
706 fbackup(1706): enter the name of the restart file (return to exit)\n
707 fbackup(1707): unable to create restart file %s for writing\n
708 fbackup(1708): unable to open restart file %s for reading\n
709 fbackup(1709): out of virtual memory\n
710 fbackup(1710): out of virtual memory\n
712 fbackup(1712): restarting session begun on %s\n
713 fbackup(1713): of graph file %s at level %d\n
714 fbackup(1714): resuming at file number %d on volume number %d\n
715 fbackup(1715): unable to create volume header file %s\n
801 fbackup(1801): out of virtual memory\n
802 fbackup(1802): out of virtual memory\n
$set 2
1 fbackup(2001): reader %d: shmat failed for memory segment\n
2 fbackup(2002): reader %d: unable to open file %s\n
3 fbackup(2003): reader %d: unable to lseek file %s\n
4 fbackup(2004): reader %d: 1st read error, 
5 fbackup(2005): reader %d: 2nd read error, 
6 fbackup(2006): reader %d: received a sig with no work to do\n
7 fbackup(2007): reader %d: aborting\n
8 fbackup(2008): reader %d: unable to read file %s,\n  file has been locked\n
$set 3
1 fbackup(3001): shmat failed for the memory segment for the writer; errno: %d\n
2 fbackup(3002): out of virtual memory\n
3 fbackup(3003): normal EOT\n
4 fbackup(3004): writer aborting\n
5 fbackup(3005): WARNING: file number %d was NOT backed up\n
6 fbackup(3006): writer lost synchronization, fno=%d, filenum=%d\n
7 fbackup(3007): WARNING: File number %d (%s)\n
8 \twas active during attempt number %d\n
9 fbackup(3009): WARNING: File number %d (%s)\n
10 \twas not successfully backed up\n
11 fbackup(3011): WRITE ERROR while writing a checkpoint EOF,
12 fbackup(3012): WRITE ERROR while writing checkpoint record,
13 fbackup(3013): WRITE ERROR while writing data record,
14 fbackup(3014): WRITE ERROR: could not write to the output file\n
15 fbackup(3015): block special output devices aren't supported\n
16 fbackup(3016): hit return when volume %d is ready on %s?\n
17 fbackup(3017): the tape is not at the beginning, rewinding it\n
18 fbackup(3018): output file %s is an illegal type\n
19 fbackup(3019): would you like to enter a new output file?\n
20 fbackup(3020): please enter the new file's name:\n
21 fbackup(3021): out of virtual memory\n
22 fbackup(3022): would you like to continue this session?\n
23 fbackup(3023): please ensure that volume %d is ready on %s\n
24 fbackup(3024): writing volume %d to the output file %s\n
25 fbackup(3025): write error at the beginning of the volume\n
26 fbackup(3026): ioctl error, can't query outfile\n
27 fbackup(3027): ioctl error, can't query outfile\n
28 fbackup(3028): WRITE ERROR while writing a checkpoint EOF,
29 fbackup(3029): WRITE ERROR while writing checkpoint record,
30 fbackup(3030): WRITE ERROR while writing data record,
32 fbackup(3032): could not open output file %s\n
33 fbackup(3033): This output file is an illegal type; please specify a 
34 UCB mode device\n\twith no-rewind, or an AT&T mode device with 
35 rewind set\n
36 fbackup(3036): could not position media %s to beginning\n
37 fbackup(3037): could not open output file %s\n
43 fbackup(3043): WRITE ERROR while writing data record,
44 fbackup(3044): ioctl error, can't query outfile\n
45 fbackup(3045): WRITE ERROR while writing data record,
46 fbackup(3046): hit return when volume %d is ready on %s?\n
47 fbackup(3047): could not open output file %s\n
48 fbackup(3048): could not position media %s to beginning\n
49 fbackup(3049): hit return when volume %d is ready on %s?\n
50 fbackup(3050): could not open output file %s\n
51 fbackup(3051): hit return when volume %d is ready on %s?\n
52 fbackup(3052): could not open output file %s\n
54 fbackup(3054): could not open output file %s\n
101  at media record %d\n
102 fbackup(3102): attempting to make this volume salvagable\n
103 fbackup(3103): rewrite this volume from the beginning?\n
104 fbackup(3104): writing 2 EOFs and rewinding the tape\n
105 fbackup(3105): writing 2 EOFs and rewinding the tape\n
106 fbackup(3106): please mount a good tape\n
107 fbackup(3107): write error on a EOF in the %s\n
108 fbackup(3108): write error on a record in the %s\n
109 fbackup(3109): write error on an EOF in the %s\n
110 fbackup(3110): write error on a record in the %s\n
111 fbackup(3111): write error on a record in the %s\n
112 fbackup(3112): write error on a EOF in the %s\n
113 fbackup(3113): write error on a record in the %s\n
114 fbackup(3114): write error on an EOF in the %s\n
115 fbackup(3115): write error on a record in the %s\n
116 fbackup(3116): write error on a record in the %s\n
117 fbackup(3117): write error on a record in the %s\n
118 fbackup(3118): write error on a record in the %s\n
119 fbackup(3119): write error on a record in the %s\n
120 fbackup(3120): write error on a record in the %s\n
121 fbackup(3121): write error on a record in the %s\n
122 fbackup(3122): write error on a record in the %s\n
123 fbackup(3123): could not read the previous checkpoint record\n
124 fbackup(3124): write error on a record in the %s\n
125 fbackup(3125): write error on an FSM in the %s\n
126 fbackup(3126): write error on a record in the %s\n
127 fbackup(3127): write error on a record in the %s\n
128 fbackup(3128): write error on an EOF in the %s\n
129 fbackup(3129): write error on a record in the %s\n
130 fbackup(3130): write error on a record in the %s\n
202 fbackup(3202): this is volume %d OF THIS SESSION!\n\trejecting this volume\n
203 fbackup(3203): volume %d has been used %d time(s)\n
204 fbackup(3204): volume used more than maximum, do you want to use this volume anyway?\n
205 fbackup(3205): unable to read a volume header\n
301 fbackup(3301): could not open output file %s\n
303 fbackup(3303): ioctl error, can't query outfile\n
304 fbackup(3304): magtape operation error\n
305 fbackup(3305): could not open output file %s\n
306 fbackup(3306): this is volume %d OF THIS SESSION!\n\trejecting this volume\n
307 fbackup(3307): volume %d has been used %d time(s) (maximum: %d)\n
308 fbackup(3308): do you want to use this volume anyway?\n
309 fbackup(3309): unable to read a volume header\n
310 fbackup(3310): enter '%s' when volume %d is ready on %s,\n or '%s' to discontinue:\n
$set 4
1 fbackup(4001): automatic '%s'\n
2 fbackup(4002): please respond with: '%s' or '%s'\n
3 fbackup(4003): semaphore op failed: op=%d\n
4 fbackup(4004): function mymalloc could not allocate space\n
