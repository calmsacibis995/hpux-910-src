$set 1
1 usage: %s dest ...\n
2 accepting
3 destination "%s" non-existent
4 destination "%s" has disappeared!
5 destination "%s" was already accepting requests
6 destination "%s" now accepting requests\n
7 spool directory non-existent
$set 2
1 usage: cancel id ... printer ...\n
2 cannot restart class
3 "%s" is not a request id or a printer
4 printer "%s" has disappeared!
5 printer "%s" was not busy
6 request "%s-%d" cancelled\n
7 your printer request %1$s-%2$d was cancelled by %3$s.
8 request "%s-%d" cancelled\n
9 request "%s-%d" cancelled\n
10 your printer request %1$s-%2$d was cancelled by %3$s.
11 spool directory non-existent
12 request "%s-%d" not cancelled: not owner\n
13 Unable to execute the remote cancel command %s.\n
14 Error %d occured.\n
15 "%s" is not a printer or a class
16 You must have root capability to use this option\n
22 usage: cancel id ... printer ... -a -e -uuser\n
23 requests may be canceled based on the following options\n
24 \tid      - request id\n
25 \tprinter - printer to cancel request on\n
26 \t-a      - all requests for the user on the specified printer\n
27 \t-e      - everything in the printer queue\n
28 \t-uuser  - the specified user(s) on the specified printer\n
29 Maximum number of remote requests exceeded. Request "%s-%d" not cancelled.\n
30 Maximum number of remote requests exceeded. Request "%s" not cancelled.\n
31 \t-i      - cancel only local requests\n
$set 3
1 %b %2d %H:%M
$set 4
1 usage: %s [-c] [-r[reason]] printer ...\n
2 unknown option "%s"
3 printer "%s" non-existent
4 no printers specified
5 printer "%s" has disappeared!
6 printer "%s" was already disabled
7 request "%s-%d" cancelled\n
8 reason unknown
9 printer "%s" now disabled\n
10 your printer request %1$s-%2$d was cancelled by %3$s.
11 spool directory non-existent
12 request "%s-%d" not cancelled: not owner\n
$set 5
1 usage: %s printer ...\n
2 enabled
3 printer "%s" non-existent
4 printer "%s" has disappeared!
5 printer "%s" was already enabled
6 printer "%s" now enabled\n
7 spool directory non-existent
$set 6
1 spool directory non-existent
2 acceptance status of destination "%s" unknown
3 can't accept requests for destination "%1$s" -\n\t%2$s
4 bad priority %d
5 no such printer or class "%s"
6 can't open %s file in CLASS directory
7 can't create request file %s
8 cannot create temp file %s
9 %s destination "%s"illegal
10 can't open default destination file
11 no system default destination
12 default destination "%s" non-existent
13 can't read current directory
14 can't open file %s
15 can't open new sequence number file %s
16 too many options for interface program
17 Warning: clusterserver name not found
18 destination "%s" non-existent
19 bad default priority in %s/%s for printer: %s
20 can't access file "%s"
21 "%s" is a directory
22 file "%s" is empty
24 request not accepted
25 %d files
26 %d file
27  and 
28 standard input
29 request id is %1$s (%2$s%3$s%4$s)\n
30 request id %s failed to enqueue\n
31 standard input is empty
34 too many files - break up the job\n
$set 7
1 printer "%1$s" is not a member of class "%2$s"
2 can't open member file
3 can't open class file
4 corrupted member file
5 printer "%1$s" already in class "%2$s"
6 can't create new member file
7 can't create new class file
8 can't create new printer status file
9 can't create new acceptance status file
10 can't create new interface program
11 can't create new request directory
12 printer status entry for "%s" has disappeared
13 destination "%s" non-existent
14 printer "%s" non-existent
15 can't create printer "%s" -- it is an existing class name
16 can't create class "%s" -- it is an existing printer name
17 "%s" is an illegal destination name
18 new printers require -v and either -e, -i or -m
19 keyletters "-%c" and "-%c" are contradictory
20 keyletters -e, -i and -m are mutually exclusive
21 class "%s" non-existent
22 model "%s" non-existent
23 can't access file "%.50s"
24 can't remove request directory
25 can't remove printer
26 requests still queued for "%s" -- use lpmove
27 can't remove class file
28 -"%c" and -"%c" keyletters have the same value
29 can't proceed - scheduler running
30 can't read current directory
31 can't read current directory
32 can't open system default destination file
33 new printer
34 new destination
35 illegal keyletter "%c"
36 unknown keyletter "-%c"
37 keyletter "%c" requires a value
38 LP Administrator not in password file\n
39 can't set user id to LP Administrator's user id
40 spool directory non-existent
41 usages:\tlpadmin -pprinter [-vdevice] [-cclass] [-rclass]\n
42        \t       [-eprinter|-iinterface|-mmodel] [-h|-l]\n
43        \tlpadmin -d[destination]\n
44        \tlpadmin -xdestination\n
45 status model "%s" non-existent
46 cancel model "%s" non-existent
47 No argument for ob3
48 Arguement for orc must be 'y' or 'n', not %s
49 can't create new cancel interface program
50 can't create new status interface program
51 illegal key "%s"
52        \t       [-ociinterface|-ocmmodel] [-ob3]\n
53        \t       [-osiinterface|-osmmodel] [-orc]\n
54        \t       [-ormmachine|-orpprinter]\n
55 bad priority %d
56 cnode "%s" non-existent
57 can't create interface "%s" as CDF
58 can't move interafce to specified cluster
59 a
60        \t       [-acluster_client]\n
$set 8
1 usage:\t%s dest1 dest2\n\t\t-or-\n\t%s request ... dest\n
2 LP system corrupted
3 scheduler is still running - can't proceed
4 destination "%s" non-existent
5 destinations are identical
6 total of %1$d requests moved from %2$s to %3$s\n
7 "%s" is not a request id
8 total of %1$d requests moved to %2$s\n
9 can't open request directory %s
10 can't move request %s
11 request "%s-%d" non-existent
12 can't open request file %s
13 can't move request %s-%d
14 can't move request %s-%d
15 all requests moved to 
16 destination %s is not accepting requests\n
17 move in progress ...\n
18 spool directory non-existent
$set 9
1 attenmpted to open a file that had been removed. seqno is %d\n
2 Unable to open and lock "%s"\n
3 can't open CLASS directory
4 can't open %s file in CLASS directory
5 non-existent printer %s in class %s
6 can't open request directory %s
7 can't open temporary output queue
8 can't unlink old output queue
10 ***** STOPPED  %s *****\n
11 disabled by scheduler: 
12 Can't execute printer interface program (errno=%d)\n
13 \tCHILD: %s  %s %d %.4x\n
14 can't write to %s
15 can't open %s
16 can't reopen stderr
17 Can't access FIFO
18 can't open FIFO
19 can't open MEMBER directory
20 can't open %s file in MEMBER directory
21 error code %d in request %s-%d on printer %s
22 printer request %s-%d has been printed on printer %s
23 error in printer request %s-%d
24 disabled by scheduler: login terminal
25 non-existent printer %s in PSTATUS
26 \tFIFO: %s  %s\n
27 lpsched: cannot create log
28 ***** LP LOG: %s *****\n
29 FIFO: '%s' ?
30 can't open request file "%s"
31 can't create temporary request file "%s"
32 can't create request file "%s"
33 can't move datafile "%s"
35 can't open temporary request file "%s"
36 can't get the entry for structure priority "%d"
37 entry gone from printer status file
38 spool directory non-existent
39 LP Administrator not in password file
40 LP Administrator not in secure password file
41 can't set LP Administrator's audit flag
42 can't set user id to LP Administrator's user id
43 can't fork
44 kill: invalid pid %d using %s\n
45 unexpected wait return\n
46 \tFIFO: Destinations --\n
47 \tFIFO: %s: %s %s %s %s\n
48 p
49 c
50 enabled
51 not enabled
52 busy
53 not busy
54 not printing
55 printing on %s
56 \tFIFO: \t%d: %d %s %s time:%d %s host %s\n
57 seqno(3)
58 seqno(4)
59 \tFIFO:\t%d: %d %s %s time:%d\n
60 \tFIFO: Printers --\n
61 pid and id unknown
62 pid=%d, id=%s-%d
63 \tFIFO: \t%s %s\n
64 \tFIFO: \t\t%s\n
65 \tFIFO: Classes --\n
66 \tFIFO: \t%s\n
68 usage: %s [-v] [-a]\n
69 unknown option "%s"
70 pipe failed to connect
71 close of write pipe failed
72 scheduler could not be started
73 scheduler was already active
74 scheduler is running
75 Can't access SCHEDLOCK
$set 10
1 spool directory non-existent
2 scheduler stopped\n
3 scheduler not running
$set 11
1 unknown option "%s"
3 system default destination: %s\n
4 no system default destination\n
6 Unable to execute the remote status command %s.\n
7 Error %d occured.\n
8 unable to malloc memory to hold more requests.\n
9 "%s" not a request id or a destination
13  on %s
14 
16  from %s
17 %-*s %-*s priority %d  %s%s%s\n
18 (standard input)
19            
20 %-4d copies
21 \t%-*s          %s
22  %*ld bytes\n
23  ??? bytes\n
24 destination "%s" non-existent
25 %1$s accepting requests since %2$s\n
26 %1$s not accepting requests since %2$s -\n\t%3$s\n
27 printer "%s" non-existent
28 (login terminal) 
29 now sending %s-%d to %s.  enabled since %s
30 now printing %1$s-%2$d.  enabled since %3$s
32 is idle.  enabled since %1$s
33 disabled since %1$s -\n\t%2$s
34 printer %1$s %2$s%3$s\n
35 \tfence priority : %d\n
36 MEMBER directory has disappeared!
38 device for %1$s: %2$s
39 printer "%s" has disappeared!
40 CLASS directory has disappeared!
41 class "%s" non-existent
42 members of class %s:\n
43 class "%s" has disappeared!
44 scheduler is running\n
45 scheduler is not running\n
46 spool directory non-existent
47 Not enough room for all the user names\n
48 User name not added is %s\n
53 can't open %s file in MEMBER directory
54 No device specified in MEMBER directory
55 Unable to stat the device  %s
56 Unable to get the current time
57 execlp failed
60     remote to: %1$s on %2$s \n
$set 12
2 can't open output queue file
$set 13
2 can't open printer status file
$set 14
2 can't open acceptance status file
$set 15
1 usage: %s [-r[reason]] dest ...\n
2 unknown option "%s"
3 destination "%s" non-existent
4 no destinations specified
5 destination "%s" has disappeared
6 reason unknown
7 destination "%s" will no longer accept requests\n
8 destination "%s" was already not accepting requests\n
9 spool directory non-existent
$set 16
1 \nlp: %s\n
$set 17
2 can't open printer status file
$set 18
1 spool directory non-existent
2 Your name is too long
3 The login name "-all" is reserved
4 Too many users
6 Too many requests
7 rcancel: No printer was specified
8 usage: rcancel [-a] [printer] [-uuser] [ids]\n
$set 19
1 spool directory non-existent
2 Too many users
3 Too many requests
4 rlpstat: No printer was specified
5 usage: rlpstat [-dprinter] [-uuser] [ids]\n
$set 20
3 spool directory non-existent
4 can't proceed - scheduler running
5 no such printer "%s"
6 fence value must be 0 to 7
7 usage : %s printer fence\n
$set 21
1 spool directory non-existent
2 acceptance status of destination "%s" unknown
3 can't accept requests for destination "%s" -\n\t%s
4 scheduler is not running. alteration failed.
5 new request id %s-%d failed to enqueue\n
6 new request id is %s-%d\n
7 failed to enqueue\n
8 no such request "%s-%d"
9 request "%s-%d" not altered: not owner
10 can't open new sequence number file "%s"
11 illegal keyletter "%c"
12 destination "%s" non-existent
13 keyletter not expecting argument "-%c%s"
14 too many options for interface program
15 bad priority "-%c%s"
16 unknown keyletter "-%c"
17 usage : lpalt id [-ddest] [-m] [-nnumber] [-ooption]\n\t[-ppriority] [-s] [-ttitle] [-w]\n
18 new destination must be different than the old destination
$set 22
1 usage: %s [-ddestination] [filename]\n
2 %s: cannot open %s\n
3 %s: logfile format error\n
4 %s: memory allocation error\n
5 performance analysis is done from %h.%d '%y %H:%M
6  through %h.%d '%y %H:%M\n
7 ---printers     ----wait----   ---print---    ---bytes---   -sum- num_of\n
8     /classes--     AV     SD     AV     SD      AV     SD      KB requests\n
