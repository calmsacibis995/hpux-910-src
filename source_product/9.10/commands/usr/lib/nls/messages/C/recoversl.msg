$set 1
1 \n%s: Checking shared libraries\n
2 
3 NOT OK\n\n
4 One or more critical shared libraries cannot be found.\nRecovery of shared libraries requires update media and\na device capable of reading the media.  If you want to\nrecover these shared libraries, this program will assist\nyou in doing so.\n\nContinue (yes/no)? 
5 You have been put into /bin/sh.  When you exit,\nthe system will continue to boot\n\nTo get back to this program, type /etc/recoversl\n
6 What kind of update media do you have:\n1. Cartridge, Nine-track, or DAT tape\n2. CD-ROM\n\nPlease choose 1 or 2: 
7 If you have more than one update tape, use the first tape.\nPlease load tape and press <return> when ready\n
8 Recovery was unsuccessful.  Check the following:\n - Correct tape\n - Tape drive on and ready\n - Tape loaded and online\n - Device file correct\n
9 \nRecovery was successful.  The system will now reboot\n
10 If you have more than one update disk, use the first disk.\nPlease insert disk and press <return> when ready\n
11 Mount of CD-ROM file system failed.  Check the following:\n - Disk drive on and ready\n - Disk inserted\n - Device file correct\n
12 %d errors occurred during recovery.  Check the following:\n - Correct CD-ROM\n
13 \nRecovery was successful.  It is suggested that you now reboot the system\n
14 \nA device file is required for the %s.\nIf one does not exist, it will be created for you.\n\nDoes a suitable device file already exist\non this system (yes/no/quit)? 
15 Enter full pathname of device file: 
16 Please enter the major and minor numbers\nfor the block special file: 
17 device file: %s
18 : not a block special file\n
19 : not a character special file\n
20 \nIs this correct (yes/no/quit)? 
21 rebooting system...\n
22 \tfixed permissions on %s\n
23 y
24 n
25 q
26 You must be super-user to use this program\n
27 tape
28 CD-ROM
29 Please enter the major and minor numbers\nfor the character special file: 
30 Could not find tar in either /bin or /usr/bin!\n
