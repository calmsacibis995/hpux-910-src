$set 1
1 -a and -D cannot both be specified\n
2 -A and -D cannot both be specified\n
3 -A and -d cannot both be specified\n
4 usage: audusr [[-a user]...] [[- d user]...] [-AD]\n
5 password file is locked, cannot do any update\n
6 cannot create the temp password file, no update\n
7 cannot open the password file /etc/passwd\n
8 cannot open the temp password file /etc/ptmp\n
9 cannot write to the temp password file /etc/ptmp\n
10 cannot stat the password file\n
11 one or more input user(s) do not exist, no update\n
12 number of input users is more than the maximum allowed\n
13 auditing system is unchanged, no users updated\n
14 cannot suspend auditing on this process, exiting\n
15 you do not have access to the auditing system\n
16 usage: audsys [-nf] [-c file -s kbytes] [-x file -z kbytes]\n
17 illegal input
18 warning: /.secure/etc/audnames does not exist\n
19 cannot open and lock /.secure/etc/audnames;\nverify that the auditing system has been installed properly,\nand the /.secure/etc directory exists, prior to running this command\n
20 warning: cannot read badly formatted /.secure/etc/audnames\n
21 created/repaired /.secure/etc/audnames\n
22 cannot create audit file: %s\n
23 audit file %s, is not an empty file\n
24 audit file %s, is not a regular file (or CDF)\n
25 internal error\n
26 -n and -f cannot be both specified\n
27 -n option must be specified to turn auditing on\n
28 auditing system is already on, input arguments ignored\n
29 -c and -s options must be specified together\n
30 auditing system unchanged\n
31 current
32 next
33 %s audit file %s\nexceeds its AFS, select a different one\n
34 %s audit file %s\ninsufficient space available on audit file filesystem,\nspecify a different audit file or select a smaller AFS\n
35 %s audit file %s\nfilesystem exceeds minfree,\nspecify an audit file on a different filesystem\n
36 unknown current audit file in use:
37 unknown next    audit file in use:
38 -c must be specified to select a current file\n
39 -x must be specified to select a next file\n
40 unknown next audit file was in use:
41 -x and -z options must be specified together\n
42 audit file directory %s doesn't exist\n
43 warning: audit file %s doesn't exist\n
44 Audit files:\n
45 current file:
46 next    file:
47 ** unknown **
48 next    file: none\n
49 next    file:        ** no data available **\n
50 current file:        ** no data available **\n
51 statistics-     afs Kb  used Kb  avail %%    fs Kb  used Kb  avail %%\n
52 current and next audit files set to same target\n
53 auditing system started\n
54 auditing system halted\n
55 created audit file:
56 other input arguments are ignored\n
57 no available next audit file\n
58 auditing system is currently off\n
59 auditing system is currently on\n
60 next file has been reset to <null> \nno next audit file is available to the system\n
101 All users 
102 will be audited\n
103 will not be audited\n
104 audsys: cannot suspend auditing - cannot continue
105 audsys: current audit file is changed to
106 audsys: next audit file is changed to
107 audsys: input audit file is not an empty regular file\n
108 audsys: unable to create audit file
109 audsys: auditing system shut-down\n
110 audsys: auditing system started\n
111 audsys: unknown current audit file in use 
112 audsys: unknown next    audit file in use 
113 audsys: reset next file to NULL\n
114 audsys: current and next file set to same file (no-next)\n
115 audsys: unable to open/lock /.secure/etc/audnames\n
116 audsys: unknown internal error\n
117 audsys: internal error: shut-down failed\n
118 audsys: internal error: /.secure/etc/audnames update failed\n
119 audsys: internal error: could not update kernel\n
120 audsys: could not find /.secure/etc/audnames\n
121 audsys: detected bad /.secure/etc/audnames\n
122 audsys: corrected /.secure/etc/audnames\n
123 audsys: could not repair /.secure/etc/audnames\n
124 audsys: auditing system unchanged\n
125 audsys: current file is 
126 audsys: next    file is 
127 audsys: expected audit file not found (deleted?)
