$ @(#) $Header: cue.msg,v 70.1 92/07/28 17:35:52 ssa Exp $
$******************************************************************************
$
$ Hewlett-Packard Character-terminal User Environment (CUE)
$ Copyright (c) 1991 Hewlett-Packard Company
$
$ FILE: cue.msg
$
$ DESCRIPTION:
$	Contains all screen text for the CUE product.
$	Used to generate the American English, C, and n-computer
$	HP NLS message catalog for CUE (cue.cat).
$
$ NOTES:
$	To generate cue.cat from this file enter the following command:
$		gencat cue.cat cue.msg 
$
$ WARNING:
$	DO NOT USE message numbers above 1000 except in $set2, because
$	the message numbers in set2 MUST be unique.  Messages in set2
$	are used in the cuesession script like this:
$		/usr/bin/dumpmsg /usr/lib/nls/$LANG/cue.cat | grep 1003
$	Thus, if there is more than 1 message with this message number
$	the grep would print both messages in the script.
$
$******************************************************************************
$ 
$ General comments which apply to all messages:
$   All messages should be no wider than 75 columns so that users can
$ easily view them. Wider messages will still work, but will go beyond 
$ the right edge of the display and the users will have to use horizontal 
$ scrolling to read a message.
$ 
$set1 Core Screen Text
1 Login Window
2 Hewlett-Packard
3 Character-terminal User Environment (CUE)
4 Username
5 Password
$ 
$ Do not translate the & character in any of the following messages. 
$ Dialog Manager uses this notation to designate the character that 
$ usres enter to signal their item selection from a menu, instead of using 
$ the arrow keys.  For example the next entry (msg #6) instructs DM to 
$ enable the user enter character C to select Configuration item from the
$ main menu instead of using the arrow keys to highlite their selection.
$ 
6 &Configuration
7 Select the &Language for the Session
8 Select the &Type of Session to Start
9 &Help
10 &Getting Started with CUE
11 How to Change the &Language of your Session
12 How to Change the &Type of your Session
13 &Start Session
14 &End Session
$ 
$ The following comments apply to all messages marked as "FUNCTION KEY LABLE".
$ The extra spaces in these messages is needed for the proper alignment of 
$ function key lables. For example message 15 is a function key label
$ such that End appears on the first line and Session on the second line
$ (directly below End) of the function key label.  In general, each function
$ key lable can have two lines. Each line in the label is 8 columns wide.
$ 
$ The following two messages (15 and 16) are FUNCTION KEY LABELS.
$ 
15 End     Session
16 MENUBAR  on/off
17 Language Menu
18 &Ok
19 &Clear
20 Session Menu
21 tsm   - Terminal Session Manager
22 keysh - Easy, Context-Sensitive, Function Key Shell
23 sh    - Bourne Shell
24 csh   - C Shell
25 ksh   - Korn Shell
26 Error Message Window
27 Help Message Window
28 Clear &Fields
$ 
$ The following two messages (29 and 30) are FUNCTION KEY LABELS.
$ 
29 Start   Session
30 Clear   Fields
31 Please select the native language for the screen text.
$ 
$ The following two messages (32 and 33) are displayed together at once;
$ in other words, they should be treated as one message. The translation
$ should fit in two lines.
$ 
32 Use the arrow keys to position the cursor and the RETURN key to make
33 a selection.  Press the Ok function key (f8) when you are finished.
34 Please select the type of work session you want.
$
$ The following two messages (35 and 36) are FUNCTION KEY LABELS.
$
35 Ok
36 Clear
$
$ In the following message (37), the "<==" character sequence is a simple
$ pointing mechanism. It can be omitted from a translation, if it would make
$ it too complicated to understand. Also, a local version of a pointing
$ mechanism can be used instead, as long as the width of the message does
$ not exceed the 75-column limit.
$
37 <== your entry here will be invisible
$
$ Exclamation marks (!) are optional in the following message (38).
$ They are used for emphasis. They can be omitted or translated to a
$ suitable local character for that purpose.
$
38 Don't forget to End your Session !!!
$
$ The following message (39) can be completely translated, including the
$ colon (:) character.
$
39 Welcome:
40 Press the MENUBAR function key (f4) to select the items above
41 Use arrow keys or SHIFT+arrow key to view off-screen data.
42 Press the Ok function key (f8) when you are finished.
$ The following two messages (43 and 44) are FUNCTION KEY LABELS.
43 Help
44 Select

$set2 Messages from the script, cuesession
$ Please leave the punctuation as it is.
1001 Please wait while your session is started - 
1002 Cannot start the session type selected - 
1003 CUE Warning: Home directory is "/".
1004 To quit, type "exit" or simultaneously press the keys: CTRL d.
1005 To exit TSM, use the Menu function key (f8) to get to the Quit menu.

$
$ Messages in the following set are in the same format as they would appear
$ on the screen. The general format is of each message is:
$   <cat1> :
$   <cat2> :
$   <cat3> :
$   <cat4> :
$ wher <cat1> represents a category; such as Name, or Purpose.
$ Note that each category (Name, Purpose, Invoke, or any other) should start on
$ a new line. Trnaslations for each category can be separated into lines by
$ using "\n" character sequence.  In such cases, starting with the second line,
$ enough blanks are prepended so that the left edge is lined up and indented
$ (look at the format of Purpose section for message 1 below as an example).
$
$set3 Help Messages
1 Name   : OK\n\
Purpose: Allow the user to return to the previous window\n\
         after the selections are made\n\
Invoke : Enter carriage return or press SELECT function key (f3)\n\
Skip   : Use tab or arrow keys to go to another field.

2 Name   : Clear\n\
Purpose: Clear current selections and restore selections\n\
         that existed at the time this menu was entered\n\
Invoke : Enter carriage return or press SELECT function key (f3)\n\
Skip   : Use tab or arrow keys to go to another field.

3 Name   : Username\n\
Purpose: Enter your Username here\n\
Skip   : Use tab or arrow keys to go to another field\n\
         or press MENUBAR to make menubar selections\n\
Details:\n\
Enter your Username.  You may enter up to 8 characters.\n\
If you make a mistake, you can correct it by using the\n\
backspace/delete key or by selecting the Clear Fields\n\
pushbutton, which will erase both your Username and\n\
Password entries.\n\n\
When you are finished, enter a carriage return, a tab key,\n\
or an arrow key to get to the next field or press the\n\
MENUBAR function key (f4) to make menubar selections.

4 Name   : Password\n\
Purpose: Enter your password here\n\
Action : Enter carriage return after the password to\n\
         start a session now.\n\
Skip   : Use tab or arrow keys to go to another field\n\
         or press MENUBAR to make menubar selections\n\
Details:\n\
Enter your Password.  The characters that you type will\n\
not appear on the screen for security purposes.\n\
If you make a mistake, you can correct it by using the\n\
backspace/delete key or by selecting the Clear Fields\n\
pushbutton, which will erase both your Username and\n\
Password entries.\n\n\
When you are finished, enter a tab key or an arrow key\n\
to get to the next field, or a carriage return to start\n\
your session, or press the MENUBAR function key (f4) to make menu\n\
bar selections.

5 Name   : Start Session\n\
Purpose: CUE will validate your Username and Password\n\
         entries before starting the your session.\n\
Invoke : Enter carriage return or press SELECT function key (f3)\n\
Skip   : Use tab or arrow keys to go to another field\n\
         or press MENUBAR to make menubar selections\n\
Details:\n\
After you have entered your Username and Password and\n\
have completed making optional menubar selections,\n\
you are ready to start your session.  To select this\n\
pushbutton, enter a  carriage return or press the\n\
SELECT function key (f3).  At this time, your Username and\n\
Password will be validated.  If the your entries pass\n\
validation, your session will be started.  If the\n\
validation fails, then you should try entering your\n\
Username and Password again.  Use the tab key or the\n\
arrow keys to get back to Username field, or press the\n\
MENUBAR function key (f4) to make menu bar selections.

6 Name   : Clear Fields\n\
Purpose: Erase both entries in the Username and Password\n\
         fields.\n\
Invoke : Enter carriage return or press SELECT function key (f3)\n\
Skip   : Use tab or arrow keys to go to another field\n\
         or press MENUBAR to make menubar selections.

7 Name   : End Session\n\
Purpose: Quit the CUE login process/logout\n\
Invoke : Enter carriage return or press SELECT function key (f3)\n\
Skip   : Use tab or arrow keys to go to another field\n\
         or press MENUBAR to make menubar selections.

8 Name   : Language Menu\n\
Purpose: The native language of these screens and your\n\
         work session can be changed with this menu.\n\
Invoke : Enter carriage return or press SELECT function key (f3)\n\
Skip   : Use arrow keys to go to another menubar item\n\
         or press MENUBAR again to leave the menubar.\n\
Default: The default native language, C, will be chosen for\n\
         you if you have never made a language selection\n\
         before with CUE.\n\
Details: Your $LANG SHELL environment variable will be\n\
         changed according to your selection.

9 Name   : Session Type Menu\n\
Purpose: The type of session that you use to do your\n\
         work can be changed with this menu\n\
Invoke : Enter carriage return or press SELECT function key (f3)\n\
Skip   : Use arrow keys to go to another menubar item\n\
         or press MENUBAR to leave the menubar.\n\
Default: The default session, Bourne shell - /bin/sh, will\n\
         be chosen for you if you have never made a session\n\
         type selection before with CUE.

10 Name   : Getting Started with CUE\n\
Purpose: This will provide on-line help on general\n\
         CUE usage such as how to start a work session\n\
         and menu navigation\n\
Invoke : Enter carriage return or press SELECT function key (f3)\n\
Skip   : Use arrow keys to go to another menubar item\n\
         or press MENUBAR again to leave the menubar.

11 Name   : Language Selector\n\
Purpose: The native language of the screens and text can\n\
         be changed with this menu\n\
Select : Use arrow keys, SHIFT+arrow key (page up/down),\n\
         and Prev/Next (horizontal scrolling) to position\n\
         the cursor over the language of your choice.\n\
         Then, press carriage return or SELECT to make a\n\
         selection\n\
Default: The default native language, C, will be chosen for\n\
         you if you have never made a language selection\n\
         before with CUE.\n\
Details: Your $LANG SHELL environment variable will be\n\
         changed according to your selection.\n\
Return : Press the Ok function key (f8) or use the TAB key to position\n\
         the cursor to the Ok pushbutton and then enter a\n\
         carriage return to establish the language change\n\
         and return to the previous window.

$
$ Maintain formating by observing the number of space characters
$ in the Details portion of message 12.
$
12 Name   : Session Selector\n\
Purpose: The type of session that you use to do your\n\
         work can be changed with this selector\n\
Select : Use arrow keys, SHIFT+arrow key (page up/down),\n\
         and Prev/Next (horizontal scrolling) to position\n\
         the cursor over the session of your choice.\n\
         Then, press carriage return or SELECT to make a\n\
         selection\n\
Return : Press the Ok function key (f8) or use the TAB key to\n\
         position the cursor to the Ok pushbutton and then\n\
         enter a carriage return to establish the language\n\
         change and return to the previous window.\n\
Details: Once you have logged on you can learn more about the\n\
session type you have selected by typing: man <session type>.\n\
For example, to learn more about sh, type:\n\
         man sh\n\n\
Session Type    Description\n\
____________    _____________________________________________\n\
sh              Bourne Shell is the standard command programming\n\
                language or "shell" that executes commands from\n\
                a terminal or a file.  This will be used as\n\
                your default session type, unless you select\n\
                another session type.\n\n\
ksh             Korn Shell (ksh) is an extension to sh with\n\
                several additional features, such as history/\n\
                command editing, aliasing and command tracking,\n\
                and conditional expression capabilities.\n\n\
keysh           Keysh is an extension of the standard Korn-shell.\n\
                It uses hierarchical softkey menus and\n\
                context-sensitive help to aid users in building\n\
                command-lines, combining the power of the Korn\n\
                shell with the ease-of-use of a menu system.\n\
                And keysh is entirely data-driven, allowing its\n\
                menus and help to be easily extended as needed.\n\
                Keysh is highly recommened while you learn\n\
                HP-UX.\n\n\
csh             C-shell (csh) is a command language interpreter\n\
                that incorporates a command history buffer,\n\
                C-like syntax, and job control facilities.\n\n\
tsm             Terminal Sessions Manager (TSM) is a windowing\n\
                system for ASCII terminals.  TSM provides\n\
                rapid access to up to 10 compute sessions,\n\
                much the same as shell layers, shl(1), but\n\
                maintains the visual context and terminal state\n\
                for each session.  Each session under TSM has\n\
                complete control of the terminal and has access\n\
                to the entire display.  When you become more\n\
                familiar with HP-UX, this can become a valuable\n\
                tool.

13 Name   : Help Message Selector\n\
Purpose: Help messages can be viewed with this selector\n\
View   : Use arrow keys, SHIFT+arrow key (page up/down),\n\
         and Prev/Next (horizontal scrolling) to view\n\
         off-screen help messages.\n\
Return : Press the Ok function key (f8) or use the TAB key to position\n\
         the cursor to the Ok pushbutton and then enter a\n\
         carriage return to establish the language change\n\
         and return to the previous window.

14 Name   : Error Message Selector\n\
Purpose: Error messages can be viewed with the selector\n\
View   : Use arrow keys, SHIFT+arrow key (page up/down),\n\
         and Prev/Next (horizontal scrolling) to view\n\
         off-screen error messages.\n\
Return : Press the Ok function key (f8) or use the TAB key to position\n\
         the cursor to the Ok pushbutton and then enter a\n\
         carriage return to establish the language change\n\
         and return to the previous window.

$
$ Maintain format by preserving the extra spaces so that the message
$ is lined up in the Screen Navigation portion:
$
15 Name   : Getting Started with CUE\n\
Purpose: To help you get started with CUE\n\
View   : Use arrow keys, SHIFT+arrow key (page up/down),\n\
         and Prev/Next (horizontal scrolling) to view\n\
         off-screen help messages.\n\
Details:\n\
         The primary function of CUE is to allow you to\n\
         easily log on to your system and begin your\n\
         customized work session.  Visually attractive screens,\n\
         helpful menus, and help windows aim to help you to\n\
         customize your work session and begin working.\n\
Getting Help:\n\
         Just push the HELP function key (f1) whenever you need\n\
         more information on any field, pushbutton, or menu\n\
         item.\n\
Customizing your Work Session:\n\
         Before you enter your Username and Password, you\n\
         may configure your CUE session with:\n\n\
            o the native language of text on your screen\n\
            o the type of session you would like to use for\n\
              your work session once you log in\n\
         Press the MENUBAR function key (f4) and select Configuration\n\
         to customize these items.  When you want to leave the\n\
         menubar press the MENUBAR function key again.\n\
Starting your Session:\n\
         Once you have customized your work session, you can\n\
         enter your Username and Password.  When you enter a\n\
         carriage return in the Password field, your Username\n\
         and Password entries will be validated with those on\n\
         the computer.  If the validation passes, your work\n\
         session will be started.  If the validation fails,\n\
         you may be given another chance to re-enter your\n\
         Username and Password.\n\
Screen Navigation:\n\
         This section will describe how to navigate in the\n\
         CUE screens.\n\n\
         Keys:\n\
            arrow up/down       - scrolls between lines\n\
            arrows left/right   - scrolls between columns\n\
            Tab                 - moves to the next field\n\
            Shift+arrow up/down - scrolls up/down by one page\n\
            SPACEBAR            - makes a selection\n\
            Return              - establish entry or selection\n\
            Back Space          - erase character\n\
            Next - scrolls horizontally by one page to the right\n\
            Prev - scrolls horizontally by one page to the left\n\
            Clear Display - used to refresh the entire screen\n\
         General Function Keys (f1-f8):\n\
            HELP (F1) - Press this function key anytime to get\n\
                        help on the item under the cursor.\n\
            SELECT (F3) - Press this function key to select a\n\
                          pushbutton, a menu item, or an item\n\
                          in a list.\n\
            MENUBAR (F4) - Press this function key to make the\n\
                           menu bar items available for selection.\n\
         Login Window Function Keys (f1-f8):\n\
            Start (F5) - Press this function key to validate your\n\
            Session      Username and Password and begin your\n\
                         work session.\n\
            Clear (F6) - Press this function key to clear entries\n\
            Fields       in both the Username and Password\n\
                         fields.\n\
            End (F8) - Press this function key to end the CUE\n\
            Session    session or logout.\n\
         Language/Session Window Function Keys (f1-f8):\n\
            Clear (F5) - Press this function key to erase the\n\
                         current entry and restore the entry\n\
                         selected when this menu was displayed.\n\
            Ok (F8) - Press this function key to establish the\n\
                      selection and return to the Login Window.\n\
         Help/Error Window Function Keys (f1-f8):\n\
            Ok (F8) - Press this function key when you have\n\
                      finished reading the messages.

16 Name   : How to Change the Language of your Session\n\
Purpose: To help you change the native language of your screens\n\
Details: Select the Configuration item in the menu bar by\n\
         entering a carriage return.  Next you will be\n\
         presented with another menu where you can select\n\
         the Language menu.\n\
Return : To return from the menu bar, press the MENUBAR\n\
         function key (f4).

17 Name   : How to Change the Type of your Session\n\
Details: Select the Configuration item in the menu bar by\n\
         entering a carriage return.  Next you will be\n\
         presented with another menu where you can select\n\
         the Language menu.\n\
Return : To return from the menu bar, press the MENUBAR\n\
         function key (f4).

18 Name   : How to Change the Language of your Session\n\
Purpose: This will provide on-line help on how to change\n\
         the native language of the CUE screens and your\n\
         work session to the native language of your choice.\n\
Invoke : Enter carriage return or press SELECT function key (f3)\n\
Skip   : Use arrow keys to go to another menubar item\n\
         or press MENUBAR to leave the menubar.

19 Name   : How to Change the Type of your Session\n\
Purpose: This will provide on-line help on how to change\n\
         the type of work session you would like to start\n\
         after you successfully enter your Username and\n\
         Password.\n\
         navigation\n\
Invoke : Enter carriage return or press SELECT function key (f3)\n\
Skip   : Use arrow keys to go to another menubar item\n\
         or press MENUBAR to leave the menubar.

$set4 NLS Language table must correlate with the nls_lang array in cue.c
$ DO NOT localize the NLS languages in parenthesis ()'s. Also, do NOT
$ change the order of entries.
$
1 Similar to DEFAULT (C) with currency symbol $ (n-computer)
2 American English (american)
3 Canadian French (c-french)
4 Danish
5 Dutch
6 British English (english)
7 Finnish
8 French
9 German
10 Italian
11 Norwegian
12 Portuguese
13 Spanish
14 Swedish
15 Icelandic
16 Japanese with Katakana phonetic alphabet (katakana)
17 Arabic
18 Northern-Africa Arabic without Hindi digits (arabic-w)
19 Greek
20 Hebrew
21 Turkish
22 Thai
23 DEFAULT language recommended by HP, defined by X/OPEN (C)
24 POSIX
25 American English for ISO 8859/1 terminal (american.iso88591)
26 Canadian French for ISO 8859/1 terminal (c-french.iso88591)
27 Danish for ISO 8859/1 terminal (danish.iso88591)
28 Dutch for ISO 8859/1 terminal (dutch.iso88591)
29 British English for ISO 8859/1 terminal (english.iso88591)
30 Finnish for ISO 8859/1 terminal (finnish.iso88591)
31 French for ISO 8859/1 terminal (french.iso88591)
32 German for ISO 8859/1 terminal (german.iso88591)
33 Italian for ISO 8859/1 terminal (italian.iso88591)
34 Norwegian for ISO 8859/1 terminal (norwegian.iso88591)
35 Portuguese for ISO 8859/1 terminal (portuguese.iso88591)
36 Spanish for ISO 8859/1 terminal (spanish.iso88591)
37 Swedish for ISO 8859/1 terminal (swedish.iso88591)
38 Icelandic for ISO 8859/1 terminal (icelandic.iso88591)
39 Czech
40 Hungarian
41 Polish
42 Rumanian
43 Serbocroatian
44 Slovene
45 Russian
46 Bulgarian
47 Simplified Chinese often used in Mainland China (chinese-s)
48 Traditional Chinese often used in Taiwan (chinese-t)
49 Traditional Chinese with the Big 5 code set (chinese-t.big5)
50 Japanese using Kanji characters and the HP-15 encoding (japanese)
51 Japanese Kanji characters and the EUC encoding (japanese.euc)
52 Korean
53 Arabic for ISO 88596 code set (arabic.iso88596)
54 Greek for ISO 88597 code set (greek.iso88597)
55 Hebrew for ISO 88598 code set (hebrew.iso88598)
56 Turkish for ISO 88599 code set (turkish.iso88599)

$set5 Error Messages
$
$ Strings of the form %<char> where <char> is a single character should
$ not be localized.
$ When multiple lines are present for a message, indent all lines after the
$ first line, so that all lines line up with the left edge of the first line.
$ Other extra spaces are also present for formating and therefore should be
$ preserved.
$
1 1 : CUE Warning.  Subsystem root : %s.
2 2 : CUE could not open the message catalog for the\n\
    language chosen.  CUE will continue in the current\n\
    language, but your work session will be in the language\n\
    you have just chosen.
3 3 : CUE Warning: Home directory is /.
4 4 : CUE Unable to change to your HOME directory, %s.\n\
    Ensure that your HOME directory entry in the password\n\
    file exists and is accessible.\n\
    See your Systems Administrator for help.
5 5 : CUE Sorry.  Maximum number of users already logged in.\n\
    Try again later.
6 6 : CUE No /etc/utmp entry.  Try using: exec cue.
7 7 : CUE No root directory for sublogin.\n\
    See your Systems Administrator for help.
8 8 : CUE No /bin/cue on root (/).\n\
    See your Systems Administrator for help.
9 9 : CUE Bad numerical group ID in the passwd file.\n\
    See your Systems Administrator for help.
10 10 : CUE Bad numerical user ID in the passwd file.\n\
    See your Systems Administrator for help.
$
$ Preserve additional spaces for proper formating and line-up of the
$ message text.
$
11 11 : CUE You need to set your password, because either\n\
you don't have a password or your password has expired.\n\n\
Here are some rules to help you choose a password:\n\
   * The password must consist of at least 6 characters.\n\
     Only the first 8 are significant.\n\
   * The password must be different from your Username\n\
     and any reverse or circular shift of it.  Upper\n\
     and lower case are considered equivalent.\n\
   * New passwords must differ from the old by at least\n\
     three characters.  Upper and lower case are equivalent.\n\
   * At least 2 characters must be letters (uppercase\n\
     or lowercase)\n\
   * At least one character must be one of the following:\n\
      - Numeric (the digits 0 through 9)\n\
      - Special (neither letters nor numeric.\n\
        For example, -, _, or $)\n\n\
   According to these rules, these are valid passwords:\n\n\
      foo-bar   $money$   Number_9   @trophy\n\n\
   Also, uppercase and lowercase are different.  Thus\n\
   these are all different passwords:\n\n\
      foo-bar   Foo-Bar   FOO-BAR\n\n\
   When you choose a password, you want to ensure that\n\
   no one can guess what you chose.  If someone knows your\n\
   password, that person may log in and access your files.\n\n\
   These are the steps that you need to perform:\n\
   1. If you don't have a password, skip this step.\n\
      If you do have a password, you will be prompted\n\
      with the following:\n\n\
        Changing password for your_username\n\
        Old password:        Enter your current password\n\
   2. When you see the following prompt, enter your\n\
      new password:\n\n\
        New password:\n\n\
   3. Next, you need to re-enter the new password to\n\
      verify it:\n\n\
        Re-enter your new password:\n\n\
   4. After this, you will be once again presented with\n\
      the CUE login screen, where you can log in with\n\
      your new password.\n\n\
    See your Systems Administrator if you need further help.
12 12 : CUE Cannot execute /bin/passwd.\n\
    See your Systems Administrator for help.
13 13 : CUE could not bind callbacks.  Verify the proper\n\
    installation of CUE and try again later.
14 14 : CUE Cannot execute /bin/cue.etc/cuesession.\n\
    See your Systems Administrator for help.
15 15 : CUE Login failed.  The username or password entry\n\
    was incorrect.
16 16 : CUE failed to retrieve message number %d from set %d\n\
    Please verify that your LANG and NLSPATH environment\n\
    variables match the cue.cat NLS message catalog.\n\
    See the man pages environ(5) and lang(5).\n\
    See your Systems Administrator for more help.
17 17 : CUE cannot get enough memory to operate properly.\n\
    See your Systems Administrator for help.
18 18 : CUE Cannot start your shell, %s.\n\
    See your Systems Administrator for help.
19 19 : CUE could not load the screen file, /bin/cue.etc/cue.dm.\n\
    Please verify the proper installation of CUE.\n
20 20 : CUE could not find this session type on your system.\n\
    Please select another session type.
21 21 : CUE can not reset uid. Please ensure setuid bit is \n\
    set for /bin/cue. See your system administrator for help.
22 22 : CUE can not open /usr/lib/nls/config. See \n\
    your system administrator for help.
23 23 : Restricted shell. Cannot run CUE from command line

