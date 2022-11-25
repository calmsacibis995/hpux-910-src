$set 1
1 (alias "%s" is unknown)\n\r
2 Cannot expand alias '%s'!\n\r
3 Use "elmalias -c" to find valid addresses!\n\r
4 '%s' is an unknown address.  Replace with: 
5 Can't build return address - hit MAX_HOPS limit!
$set 2
1 Enter alias name: 
2 already a group with that name:%s
3 already an alias for that: %s
4 Full name for %s: 
5 Enter address for %s: 
6 No address specified!
7 No message to alias to!
8 Current message address aliased to: 
11 Full name of %s: 
12 couldn't open %s to add new alias!
13 Couldn't add new alias to file %s
14 Alias commands
15 A)lias current msg,  M)ake new alias,  D)elete alias  or  Check a P)erson.
16 E)xpand and check alias,  List  U)ser  or  S)ystem aliases.
17 initializing internal tables...
18 Alias: 
21 Check for person: 
22 Group alias:\n\r%s
23 Aliased address:\n\r%s
24 not found
25 List system aliases:\r\n
26 \n\n(If the screen is messed up, please use ^L to redraw it)\n
27 \n\nPress any key to return to Alias Main Menu: 
28 Can't read system %s file
29 List user aliases:\r\n
32 Can't read user %s file
38 Invalid input!
39 Deleting alias...
40 Adding new aliases...
41 Re-reading the database in...
42 New aliases installed successfully
43 Delete aliases successfully
44 'elmalias' failed.  Please check 'aliases.text' or 'elmalias'.
45 Key you want help for : 
46 Enter key you want help for, '?' for list or '.' to leave help
47 Key : 
48 a = Add return address of current message to alias database.
49 m = Make new user alias, adding to alias database.
50 Return to the main menu.
51 p = Check for a person in the alias database.
52 s = Display system alias database.
53 That key isn't used in this section.
56 Opened %s hash file, but couldn't open data file!\n
57 d = Delete user alias, deleting from alias database when done.
58 u = Display user alias database.
59 e = Display fully expanded alias.
60 ^L = Rewrite screen.
61 Fully expand alias: 
62 Aliased address:\n\r%s
63 Press any key to continue: 
$set 3
1 initializing internal tables...
$set 4
1 %s is an illegal address!
2 \n** You appear to have a system alias file built for a different version of the\n
3 ** Elm system.  You'll need to have your administrator rebuild it as root\n
4 ** using the ``elmalias'' command (no modifications to the file are needed\n
5 ** --> just invoke `elmalias -q' as user root).\n\n
6 \n** You appear to have an alias file built for a different version of the\n
7 ** Elm system.  You'll need to rebuild it using the ``elmalias'' command,\n
8 ** or, if you'd like, I can do it directly.  Shall I (y/n) ? 
9 \n\n** I won't do it, but you will need to by hand instead: just type the\n
10 ** command ``elmalias'' at the next prompt -- no changes to the actual\n
11 ** alias text file are needed).\n\n
12 Yes\n\n** rebuilding alias database...\n\n
13 \n** problem rebuilding aliases (%d ; please fix by hand.\n\n
14 \n** aliases rebuilt, please reinvoke the Elm program.\n\n
$set 5
1 Usage: %s [-akKmz] [-f file] \n
2        %s [-s subject] <names>\n
3        %s -h\n\n
4 Warning: system created without debugging enabled - request ignored\n
5 \n\rDon't understand specifying a subject and no-one to send to!\n\n\r
6 \n\rDon't understand whom to send the mail to!\n\n\r
7 \nPossible Starting Arguments for ELM program:\n\n
8 \targ\t\t\tMeaning\n
9 \t -a \t\tArrow - use the arrow pointer regardless\n
10 \t -fx\t\tFile - read file 'x' rather than mailbox\n
11 \t -h \t\tHelp - give this list of options\n
12 \t -k \t\tSoftkeys off - disable HP 2622 terminal softkeys\n
13 \t -K \t\tKeypad&softkeys off - disable use of softkeys + keypad\n
14 \t -m \t\tMenu - Turn off menu, using more of the screen\n
15 \t -sx\t\tSubject 'x' - for batchmailing\n
16 \t -z \t\tZero - don't enter Elm if no mail is pending\n
$set 6
1  %d line(s) left : press <space> for more, or 'q' to return 
2  %d line(s) more : Press <space> for more, 'q' to return 
3  %d line(s) more 
4 ...searching for pattern "%s"...
5 Pattern not found : Press any key to continue
$set 8
1 Warning: couldn't figure out system connections...
$set 9
1 Delete alias name: 
2 no aliases for that name: %s
3 Can't delete system alias: %s
4 Can't open alias text file: %s
5 couldn't modify alias text file!
6 link failed! %s - %s
7 Can't delete multiple aliases at once
$set 10
1 Warning: domain %s uses a defunct field!!
2 Couldn't find a path to %s!
3 I Couldn't find a path to %s!
$set 11
1 invoking editor...
2 Warning: couldn't 'stat' file, perms might get mangled
3 Problems invoking editor %s!
4 Warning: new mail received...
5 Couldn't reopen tempfile.  Edit LOST!
6 Couldn't open %s for reading!  Edit LOST!
7 Couldn't seek to end of mailbox.  Edit LOST!
8 Couldn't copy temp file to mailbox!
9 \n\rCouldn't link %s to mailfile %s...\n\r
10 \n\rCouldn't internally link %s to mailfile %s...\n\r
11 \n\rYou'll need to check out %s for your mail...\n\r
12 edit changes incorporated into new mail...
13 Resynchronizing with new version of mailbox...
$set 12
1 invoking editor...
2 \r\ninvoking editor...\r\n
3 Can't invoke editor - too much options
4 Can't invoke editor - too much options\r\n
5 Can't invoke editor '%s' for composition.
6 Can't invoke editor '%s' for composition.\r\n
7 Couldn't open %s for appending
8 \n\nPlease continue message, '.' to end, %cp to list, %c? for help;\n\n
9 \n\nPlease enter message, '.' to end, or %c? <return> for help;\n\n
10 (continue)\n\r
11 (continue entering message, '.' to end)\r\n
12 \r\n(Can't find '%s' on this system!  continue)\r\n
13 \r\nPlease enter the name of the editor : 
16 (you need to use a specific command here.  Continue...)\r\n
18 \r\n(don't know what %c%c is.  Try %c? for help)\r\n
19 (Warning: couldn't append to edit buffer - %s .....Continue.)
20 \r\n<end-of-message>\n\r\n\r
21 \r\nAvailable options at this point are;\n\n
22 \t%c?\tPrint this help menu\n
23 \t~~\tAdd line prefixed by a single '~' character\n
24 \t%cb\tChange the addresses in the Blind-carbon-copy list\n
25 \t%cc\tChange the addresses in the Carbon-copy list\n
26 \t%ce\tInvoke the $EDITOR on the message, if possible\n
27 \t%cf\tadd the specified list of messages, or current\n
28 \t%ch\tchange all available headers (to,cc%s,subject)\n
29 \t%cm\tsame as '%cf', but with the current 'prefix'\n
30 \t%co\tInvoke a user specified editor on the message\n
31 \t%cp\tprint out message as typed in so far\n
32 \t%cr\tRead in the specified file\n
33 \t%cs\tChange the subject of the message\n
34 \t%ct\tChange the addresses in the To list\n
35 \t%cv\tInvoke the $VISUAL editor on the message\n\n
36 \t%c!\texecute a unix command (or give a shell if no command)\n
37 \t%c<\texecute a unix command adding the output to the message\n\n
38 \t.  \tby itself on a line (or a control-D) ends the message\n\n
39 \r\n(no filename specified for file read!  Continue...)\r\n
40 \r\n(Couldn't read file '%s'!  Continue...)\r\n
41 \r\n(Sorry, couldn't add completely: %s)
42 \r\n(added %d line(s) from file %s.  Please continue...)\r\n
43 \r\n(added %d line(s) to message.  Please continue...)\r\n
44 \r\nPanic: Can't open file for reading! \r\n
45 \r\nPanic: Can't reopen file for appending!\r\n
46 \r\n(continue entering message, please)\r\n
47 (you need to specify what message(s) to include)\r\n
48 (can't get to 'readmail' command.  Sorry...)\r\n
49 (Couldn't add the requested message.   Continue)\r\n
50 (Couldn't add the requested message completely. Continue)\r\n
51 (added %d line(s) to message...  Please continue)\r\n
52 (Interrupt.  One more to cancel this letter...)\n\r
$set 13
1 New mail has arrived!
2 %d new message(s) received
3 Command: 
4 Command: 
5 pattern not found!
6 Scan message for calendar entries...
7 Sorry - calendar function disabled
8 Bounce message
9 No mail to bounce!
10 Change mailbox
11 No mail to delete!
12 Edit mailbox
13 Mailbox is empty!
15 Forward
16 No mail to forward!
17 Group reply
18 Can't group reply to a Form!!
19 building addresses...
20 No mail to reply to!
21 Message with headers...
22 Read Message
23 Limit displayed messages by...
24 Mail
26 Next Message
27 Options
28 Print mail
29 No mail to print!
30 Quit
31 New Mail!  Quit cancelled...
32 Reply to message
33 Quit cancelled...
34 No mail to save!
35 Save Message
36 (Use '?' to list your folders)
37 No mail to tag!
38 No mail to mark as undeleted!
39 No mail to undelete!
40 Exit
41 No mail to check
42 New Current Message
43 Unknown command: Use '?' for commands
44 already at the first message!
45 No messages to read!
46 only %d message(s) selected!
47 only %d message(s)!
48 Current message number = %d\t\t%d message(s) total\n
49 Header_page = %d           \t\t%d possible page(s)\n
50 \nCurrent mailfile is %s.\n\n
51 Press any key to return: 
52 No mail to pipe!
53 Mailbox has modified! --- resyncing
54 [%d message(s) %s %s, and %d deleted]
55 [message(s) stored in your mbox]
56 [all messages deleted]
$set 14
1 ERR-UNKNOWN (%d)
2 Unknown error - %d - No description
$set 15
1 File message(s) in: 
3 Couldn't append to '%s': %s
4 Couldn't append to '%s' completely: %s
5 Message(s) saved
6 Message %d appended to file '%s'
7 Message %d saved to file '%s'
8 MAILDIR not defined.  Can't expand '%c'
9 Don't know what the value of $%s is!
10 Invalid file name.  Message is not saved.
11 Popen error.  Message is not saved.
$set 16
1 \n\rError attempting fstat on file %s!\n\r
3 could not open file %s
4 File %s already exists!  Overwriting...\n
5 You have no mail to read.\n
$set 17
1 ELM [seek] failed trying to read %d bytes into file
$set 18
1 Error %s trying to open %s to check fields!
2 Can't read the message to validate the form!
3 Couldn't open newform file for form output!
4 Failed to make temp form file %s : %s
5 Fail to make temp form file %s : %s
7 Error %s unlinking file %s
8 Error %s linking %s to %s
10 ELM [seek] couldn't read %d bytes into file (%s)
11 No form in this message!?
13 Badly constructed form.  Can't reply!
15 Can't open "%s" as output file! (%s)
16 (Failed to append to edit buffer: %s)\r\n
17 (Enter as many lines as needed, ending with a '.' by itself on a line)\n
18 (Failed to append to edit buffer: %s)\r\n
19 Failed to append to edit buffer: %s
$set 19
1 Choice: 
2  (checking) 
5 Unknown header being specified!
6 Message Header Edit Screen
7 Choose first letter of existing header, U)ser defined header, or <return>
9 How many days in the future should this message expire? 
10 That doesn't make sense!
11 Expiration date must be within eight weeks of today
12 you can't have leading white space in a header!
13 you can't have a colon as the first character!
14 You need to have a colon ending the field name!
15 You can't have white space embedded in the header name!
16 Too many people. Copies ignored !
17 Subject: 
18 Bcc: 
19 Cc: 
20 Expires: %s
21 Action: 
22 Priority: 
23 In-Reply-To: %s
24 To : %s
25 Cc : %s
26 Bcc: %s
27 Reply-To: %s
28 Subject: %s
29 Action: %s
30 Expires: %s
31 Priority: %s
32 Set current message to : 
$set 20
1 \r\nElm Header Editor.  Please choose from the following:\n\r\n
2 Choice? q%c
3 \r\nHeader to edit ('?' for choices) : q%c
4 View Headers:\r\n\n
5 Quit Header Editor\r\n
6 Help\r\n\n
7 %c\r\nI don't understand that choice!\r\n
8 How many days in the future should this message expire? 
9 You can't send messages that have already expired!\r\n
10 The expiration date must be within eight weeks of today\r\n
11 \rToo many people. Copies ignored !\r\n
12 \rToo many people. Blind Copies ignored !\r\n
$set 21
1 ELM Help System
2 Press keys you want help for, '?' for a list, or '.' to end
3 Help on key: 
5 Unknown command.  Use '?' for a list of commands...
6 $ = Force a resync of the current mailbox.  This will 'purge' deleted mail
7 ! = Escape to the Unix shell of your choice, or just to enter commands
9 | = Pipe the current message or tagged messages to the command specified
12 * = Go to the last message in the current mailbox
13 - = Go to the previous page of messages in the current mailbox
14 '=' = Go to the first message in the current mailbox
15 + = Go to the next page of messages in the current mailbox
16 / = Search for specified pattern in mailbox
17 < = Scan current message for calendar entries (if enabled)
20 a = Enter the alias sub-menu section.  Create and display aliases
21 b = Bounce (remail) a message to someone as if you have never seen it
22 c = Change mailboxes, leaving the current mailbox as if 'quitting'
23 d = Mark the current message for future deletion
24 ^D = Mark for deletion all messages with the specified pattern
26 f = Forward the current message to someone, return address is yours
27 g = Group reply not only to the sender, but to everyone who received msg
28 h = Display message with all Headers (ignore weedout list)
29 j = Go to the next message.  This is the same as the DOWN arrow
30 k = Go to the previous message.  This is the same as the UP arrow
31 l = Limit displayed messages based on the specified criteria
32 m = Create and send mail to the specified person or persons
33 n = Read the next message, then move current to next messge
34 o = Go to the options submenu
35 p = Print the current message or the tagged messages
36 q = Quit the mailer, asking about deletion, saving, etc
37 r = Reply to the message.  This only sends to the originator of the message
38 s = Save current message or tagged messages to specified file
39 t = Tag a message for further operations (or untag if tagged)
40 ^T = Tag all messages with the specified pattern
41 u = Undelete - remove the deletion mark on the message
42 x = Exit the mail system quickly
43 <return> = Read the current message
44 ^L = Rewrite the screen
45 Exit the mail system quickly
46 <number> = Make specified number the current message
47 couldn't open helpfile %s
48 Press any key to continue: 
49 Press any key to return: 
50 ^U = Undelete - remove the 'D' mark on all msgs with the specified pattern
$set 22
1 Could not open file %s for debug output!\n
2 Sorry, but you must specify what type of terminal you're on if you want to\n
3 run the "elm" program...(you need your environment variable "TERM" set)\n
4 You need a cursor-addressable terminal to run "elm" and I can't find any\n
5 kind of termcap entry for "%s" - check your "TERM" setting...\n
6 Failed trying to initialize your terminal entry: unknown return code %d\n
7 Couldn't read password entry??
8 Can't open mailbox '%s':%s\n
9 \n\r\n\rCouldn't allocate initial headers!\n\r\n
10 \n\r\n\rCouldn't show the header index!\n\r
11 \n\rPlease increase the culumn of window!\n\r
$set 23
1 Delete message(s)? (y/n) 
2 Nothing deleted
3 Delete all message(s)? (y/n) 
4 Nothing deleted
5 Keep mail in incoming mailbox? (y/n) 
6 Mailbox unchanged
7 New mail has just arrived - resyncing...
8 [keeping %d message(s), and deleting %d]
9 [keeping all messages]
15 [deleting all messages]
16 [no messages to %s, and none to delete]
17 keep
18 save
19 Couldn't append to %s: %s\n
20 Mailbox %s unchanged: %s\n
21 Error %s (%s) on stat(%s)
23 couldn't modify mail file!
24 something godawful is happening to me!!!
25 saved mail in %s
26 link failed! %s - %s
27 ..keeping empty mail file '%s'..
29 \n\rError encountered while attempting to create lock file %s;\n\r
30 Can't create lock file!  I need write permission in "%s"\n\r
31 Mail being received...\t\twaiting...1 
32   Attempt #%d: Mail being received...waiting  
33 \nTimed out - removing current lock file...
34 Throwing away the current lock file!
35 \n\rI couldn't remove the current lock file %s\n\r
36 \n\r\n\rGiving up after %d iterations...\n\r
37 \n\rPlease try to read your mail again in a few minutes.\n\r\n\r
38 Timed out on lock file reads.  Leaving program.
$set 24
1 Already have selection criteria - add more? (y/n) n%c
2 Adding criteria...
4 Enter criteria: 
5 Don't understand "%s" as a selection criteria!
6 no items selected
7 %d item(s) selected
$set 25
1 mail not sent
2 \r\n\r\nMail Cancelled!\n\r
3 \n\rNo subject - Continue with message? (y/n) n%c
4 No subject - Continue with message? (y/n) n%c
5 \n\r\n\rMail not sent!\n\r
6 Too many people.  Copies ignored
7 Too many people.  Blind Copies ignored
8 Copy message? (y/n) 
9 Send the message to: 
10 To: 
11 Subject of message: 
12 Subject: 
13 Subject: <none>
14 Copies To: 
15 Blind Copies To: 
16 To: (%s)
17 To: %s
18           To: %s
$set 26
1 Could not create file %s (%s)\r\n
2 Could not create file %s (%s)
3 Sorry, fail to transmit : %s\n
4 couldn't find forms file!
5 Warning - couldn't append signature file: %s\r\n
6 Warning - couldn't append signature file: %s
7 Warning - couldn't append signature file completely: %s\r\n
8 Warning - couldn't append signature file completely: %s
13 mail not sent without To: header! 
14 Could not open reply file (%s)\r\n
15 Could not open reply file (%s)
16 Could not open reply file (%s)\n
17 Couldn't copy outgoing message to %s : %s\r\n
18 Couldn't copy outgoing message to %s : %s
19 Message sent using another mailer - %s\r\n
20 Message sent using another mailer - %s
21 sending mail...
23 \rmail sent!      \n\r
24 Mail sent!
25 Recall last kept message instead? (y/n) y%c
26 Recall last kept message? (y/n) y%c
27 \n\rYour options now are:\n\r
28 S)end the message, E)dit it again, change/add H)eaders or F)orget it\n\r
29 \n\rWhat is your choice? s%c
30 Message not saved, Sorry.\n\r\n\r
32 Message saved in file "$HOME/%s"\n\r\n\r
33 \n\rI'm afraid I don't understand that - please answer again...\n\r
34 Please choose one of the following options by the first character
35 E)dit the message, edit the H)eaders, S)end it, or F)orget it: s%c
36 Choose: edit H)eaders, S)end, or F)orget : s%c
37 Choose: E)dit form, edit H)eaders, S)end, or F)orget : s%c
38 Choose: E)dit msg, edit H)eaders, M)ake form, S)end, or F)orget : s%c
39 Please choose: E)dit msg, edit H)eaders, S)end, or F)orget : s%c
40 Message kept - Can be restored at next F)orward, M)ail or R)eply 
41 No fields in form!
42 Error %s encountered trying to write to %s\r\n
43 Error %s encountered trying to write to %s
44 Would you like a copy "bounced" off the remote? (y/n) 
45 Yes - Bounceback included\n\r
46 "Bounce" a copy off the remote machine? (y/n) n%c
$set 27
1 Warning: blank line in %s ignored!\r\n
2 Warning: blank line in %s ignored!
3 Warning: fail to execute backquoted command in header\r\n
4 Warning: fail to execute backquoted command in header
5 backquoted command "%s" in .elmheaders failed\r\n
6 backquoted command "%s" in .elmheaders failed
$set 28
1      Another elm is already reading this mail!\n\r
2 \n\r     [if this is in error then you'll need to remove '%s/%s%s']\n\r
3 Can't open mailbox '%s' (%s)
4 (Use '?' to list your folders)
5 Name of new mailbox: 
6 \n\rName of new mailbox: 
8 already reading that mailbox!
9 Can't open file '%s': %s
10 Resynchronizing file
11 Mailbox: %s
12 Sorry, but I can't seem to unlink your temp mail file [%s]\n\r
13 Reading in %s, message: %d
14 Reading in %s, message: 0
15 Reading message: 0
17 The temp mailbox already exists !!
18 Leave without tampering with it now...
19 \nCouldn't open file %s for use as temp mailbox;\n
20 \nCouldn't reopen file %s for use as temp mailbox;\n
21 \nCouldn't seek to %ld (end of mailbox) in %s!\n
22 Mail being forwarded to %s
23 \n\rMail file is corrupt!!  Can't read it!!\n\r\n\r
24 Couldn't copy %s file to temp mailbox: %s
25 Sorry, leave without any change with mailbox...
26 \n\r\n\rCouldn't allocate enough memory!  Failed on message #%d\n\r\n\r
27 \nCouldn't reopen %s as temp mail file;\n
28 Mail file is corrupt!!  Can't read it!!
29 Invalid file name.  Mailbox can not be changed.
30 Popen error.  Mailbox can not be changed.
$set 29
1 Select first letter of Option line, '>' to Save, or R)eturn
2 Command: 
3 Save options in .elm/elmrc...
4 Command unknown!
5 -- Elm Options Editor --
6 (use <space> to toggle, any other key to leave)
7 <space> to change
8 (<space> for next, or R)everse)
9 resorting mailbox...
10 This sort will order most-recently-sent to least-recently-sent
11 This sort will order most-recently-received to least-recently-received
12 This sort will order most-recently-added-to-mailbox to least-recently
13 This sort will order by sender name, in reverse alphabetical order
14 This sort will order messages by longest to shortest
15 This sort will order by subject, in reverse alphabetical order
16 This sort will order by reverse status - Deleted through Tagged...
17 This sort will order least-recently-sent to most-recently-sent
18 This sort will order least-recently-received to most-recently-received
19 This sort will order least-recently-added-to-mailbox to most-recently
20 This sort will order by sender name
21 This sort will order messages by shortest to longest
22 This sort will order messages by subject
23 This sort will order by status - Tagged through Deleted...
24 This is the file where calendar entries from messages are saved.
25 This is the program invoked to display individual messages (try 'builtin')
26 This is the editor that will be used for sending messages, etc.
27 This is the folders directory used when '=' (etc) is used in filenames
28 This determines if you have the mini-menu displayed or not
29 Whether to display the names and addresses on mail, or names only
30 This is where copies of outbound messages are saved automatically.
31 This is how printouts are generated.  "%s" will be replaced by filename.
32 This is used to specify the sorting criteria for the mailboxes
33 When mail is sent out, this is what your full name will be recorded as.
34 This defines whether the ELM cursor is an arrow or a highlight bar.
35 The level of knowledge you have about the Elm mail system.
36 Enter the key you want help on, '?' for a list, or '.' to exit help
37 Key : 
38 %c isn't used in this section
39 %s = Return to main screen with the option values.
40 Input is NULL, will use default value!
41 Couldn't find the folder directory -- %s
42 Do you want to create the directory (y/n)? y%c
43 Couldn't make the directory, will use default value!
44 Will use default value!
45 Use previous value
46 When mail is sent out, expands tabs to spaces or not.
47 C)alendar file       : %s
48 D)isplay mail using  : %s
49 D)isplay mail using  : %s
50 E)ditor              : 
51 E)ditor              : %s
52 F)older directory    : 
53 F)older directory    : %s
54 S)orting criteria    : %s
55 O)utbound mail saved : 
56 O)utbound mail saved : %s
57 P)rint mail using    : 
58 P)rint mail using    : %s
59 Y)our full name      : 
60 Y)our full name      : %s
61 A)rrow cursor        : %s
62 M)enu display        : %s
63 U)ser level          : %s
64 N)ames only          : %s
65 T)abs to spaces      : %s
$set 30
1 Command: 
2 %s messages that match pattern...
3 Some messages are already tagged
4 A message is already tagged
5 - Remove tag(s)? y%c
6 Enter pattern: 
7 %s %d messsage(s)
8 tagged
9 marked for deletion
10 undeleted
11 no matches - no messages %s
12 / = match anywhere in messages
13 Match Pattern: 
14 Match Pattern (in entire mailbox):
15 only occurrence is in the current message
16 searching mailbox for pattern...
17 ELM [match] failed looking %ld bytes into file (%s)
$set 31
1 \n\r\n\rCouldn't malloc %d bytes!!\n\r\n\r
$set 32
1 \n\rWarning:\n\r
2 All the Elm 'dot' files are expected to have been moved into their own\n\r
3 directory -- $HOME/.elm -- by now.  Would you like me to move them all\n\r
4 for you [if you choose not to, the files will be ignored] (y/n)? y%c
5 \n\rI've left them.\n\r
6 Line %d of your ".elm/elmrc" is badly formed:\n> %s\n
7 Error reading '.elm/elmrc' file;\n
8 Line %d: Don't know what sort key '%s' specifies!\n
9 "signature" used in obsolete way in .elm/elmrc file - ignored!\n\r
10 \t(signature should specify the filename to use rather than on/off)\n\r\n
11 Warning: bounceback is set to greater than %d (max-hops) - Ignored.\n
12 Warning: timeout is set to less than 10 seconds - Ignored.\n
13 I can't understand line %d in your ".elm/elmrc" file:\n> %s\n
14 Too many weed headers!  Leaving...\n
15 Too many weed headers - out of memory!  Leaving...\n
16 \n\rNot enough memory for default weedlist.  Leaving.\n\r
17 \n\rCan't expand environment variable '%s'\n\r
18 \n\rCan't expand environment variable '%s' - too long filename
19 Elm requires the directory -- $HOME/.elm to store your elmrc and alias files.\n\r
20 Do you want to create the directory, now (y/n)? y%c
21 \n\rOkay.\tI'll do it.\n\r
22 \n\rCouldn't make the directory.   * %s *\n\r
23 \n\rOkay.\tBut, you may have some problems later.\n\r
24 Couldn't find the folder directory -- %s\n\r
$set 33
1 Sorry - couldn't open file %s for writing (%s)
2 Sorry - couldn't append to file %s: %s
3 Are you sure you want to remail this message (y/n) ? y%c
4 Bounce of message cancelled
5 resending mail...
6 Mail resent
7 too many addresses to bounce to - mail not sent !
$set 34
1 <no editing allowed>
2 Edit outgoing message (y/n) ? 
3 ELM [seek] couldn't read %d bytes into file (%s)
4 Can't build return address - hit MAX_HOPS limit!
$set 35
1 couldn't seek %d bytes into file (%s)
2 couldn't seek %d bytes into the file (%s)
$set 36
1 Warning: saving without comments - can't get to %s
2 Can't save configuration: can't write to %s [%s]
3 Options saved in file %s
4 Failed to save "%s" ..... continue
5 Couldn't save "weedlist" completely
6 Couldn't save "alternatives" completely
7 Can't save configuration: too long pathname
$set 37
1 variable 'SAVEFILE' not defined!\r\n
2 variable 'SAVEFILE' not defined!
3 couldn't append to %s : %s\r\n
4 couldn't append to %s : %s
5 couldn't read file %s: %s\r\n
6 couldn't read file %s: %s
$set 38
1 Mailbox is '%s' [Elm %s]
3 Mailbox is '%s' with %d shown out of %d [Elm %s]
4 Mailbox is '%s' with %d message(s) [Elm %s]
5 You can use any of the following commands by pressing the first character;
6 D)elete or U)ndelete mail,  M)ail a message,  R)eply or F)orward mail,  Q)uit
7 To read a message, press <return>.  j = move down, k = move up, ? = help
8 |=pipe, !=shell, ?=help, <n>=set current to n, /=search pattern
9 A)lias, C)hange mailbox, D)elete, F)orward, G)roup reply, M)ail, N)ext,
10 O)ptions, P)rint, R)eply, S)ave, T)ag, Q)uit, U)ndelete, or eX)it
$set 39
1 Only %d messages!
2 you can't read THAT message!
3 ELM failed seeking %d bytes into file (%s)
4 Can't create pipe to %s [%s]
5 %s(message addressed to %.60s)\n
6 %s(message addressed to %s)\n
7 \n(** This message is tagged Private
8 \n(** This message is tagged Company Confidential
9 \n(** This message is tagged Urgent
10 , Urgent
11  and Urgent
12 \n(** This message has Expired
13 , and has Expired
14 Sync error: can't re-open mailbox!!
15  Please press <return> to return to Main : 
$set 40
1 Command: pipe
2 Command: system call
3 can't scan for calendar entries!
4 Command: bounce message
5 message marked for deletion
6 Message marked for deletion.  Command ? 
7 Command: forward message
8 Command: group reply
9 Command: Mail message
11 Queued for printing.  Command ? 
12 Command: reply to message
13 Command: save message
14 message tagged
15 Message tagged.  Command ? 
16 message undeleted
17 Message undeleted.  Command ? 
18 Request (return to I)ndex page) ? 
$set 41
1   Read     Msg
2   Mail     Msg
3   Reply  to Msg 
4   Save     Msg
5  Delete    Msg   
6 Undelete   Msg
7  Change  Mailbox
9  Delete/Undelete
10  Print     Msg
11   Quit     Elm
12  Alias  Current
13  Check   Person
14  Display System
15  Delete  Alias
16  Make    Alias
17  Return  to Main 
18   Yes
19    No
20   Next    Page  
21   Next    Msg   
22   Prev    Msg   
25   Send    Msg   
27   Mail  Directry
28   Home  Directry
29 Incoming Mailbox
30  Cancel
32  Forget
33   Edit   Headers
34   Send
35   Edit    Form
38   Edit    Msg
41   Send
42   Edit    Msg
43   Edit   Headers
44   Send
45 Expanded Address
46  Display User
47 Subject
48  From
49  To
50 Unlimit
$set 42
1 sorting messages by %s
$set 43
1 get_token calls nested greater than %d deep!
2 possible alias loop.  an alias could not be resolved in %d expansions!
$set 44
1 (use the shell name for a shell)
2 Shell Command: 
3 \n\n(If the screen is messed up, please use ^L to redraw it)\n
4 \n\nPress any key to return to ELM: 
5 Return code was %d
6 Pipe tagged msg to: 
7 Pipe tagged msg to : 
8 Pipe current msg to: 
9 \n\nPress any key to return to ELM: 
11 Don't know how to print - option "printmail" undefined!
12 queueing...
13 Message(s) queued up to print
14 Printout failed with return code %d
15 \n\rContents of your folder directory:\n\r\n\r
16 Too many messages to print - queued up only %d message(s)
17 Too many message(s) to print or long printout command
18 Too many messages or too long piped command
$set 45
1 \nEmergency Exit taken!  All temp files intact!\n\n
2 Error: state stack corrupted
$set 46
1 \n\nStopped.  Use "fg" to return to Elm\n\n
2 Back in Elm.  (you might need to explicitly request a redraw)
3 \n\nIllegal Instruction signal!\n\n
4 \n\nFloating Point Exception signal!\n\n
5 \n\nBus Error signal!\n\n
6 \n\nSegment Violation signal!\n\n
