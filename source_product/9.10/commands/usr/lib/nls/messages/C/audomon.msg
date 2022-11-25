$set 1
1 usage: audomon [-p fss] [-t sp_freq] [-w warning] [-v] [-o output]\n
2 audomon: fss is an integer > 0 and < 100\n
3 audomon: sp_freq (minutes) is a positive real number\n
4 audomon: warning is positive integer <= 100\n
5 file system of audit file has %d%% free space left !\n
6 an attempt to switch to the backup file failed.\n
7 Must specify a backup file now !\n
8 current audit file size is %ld kilobytes!!!\n
9 an attempt to switch to the backup file failed.\n
10 audit system approaching FreeSpaceSwitch point.\n
11 free space left on file system = %d%%.\n
12 audit system approaching AuditFileSwitch point.\n
13 current audit file size = %ld kilobytes.\n
14 audit system corruption: could not open/read\n
15 audit control file - %s !\n
16 audit system corruption: couldn't find current\n
17 audit file, %s, in the control file, %s.\n
18 cannot get current/next audit files -\n
20 audomon: you do not have access to the auditing system.\n
24 audomon: cannot stat the current audit file.\n
26 audomon: cannot write to output file.\n
27 audomon: cannot stat the current audit file system.\n
