$set 1
1 The -x option is not available.\n
2 Unknown option %s\n
3 [Hit return to continue]
101 usage: ed1000 [-r] [file]\n
$set 2
1 Addr1 > addr2|First address exceeds second
2 Bad count|Nonzero count required
3 No address allowed|No address allowed on this command
4 Badly formed address
5 No match to BOTTOM|Address search hit BOTTOM without matching pattern
6 No match to TOP|Address search hit TOP without matching pattern
7 Fail|Pattern not found
8 Marks are ' and a-z
9 Undefined mark|Undefined mark referenced
10 Negative address|Negative address - first buffer line is 1
11 Not that many lines|Not that many lines in buffer
$set 3
1 Offset out-of-bounds|Offset after command too large
2 Home directory unknown
3 Mark what?|%s requires following letter
4 Bad mark|Mark must specify a letter
5 Preserve failed!
6 File preserved.
7 No write|No write since last change (:rewind! overrides)
8  HP Version %s
9 Bad register
10 At EOF|At end-of-file
11 What?|Unknown command character '%c'
12 No job control|Not using a shell with job control
101  Ed1000 : 
102  Use ? for help\n
$set 4
1 Extra chars|Extra characters at end of command
2  [Warning - %s is incomplete]\n                                   \nYour edited changes will be lost if you do not complete a successful\nwrite command before exiting.  If you were writing out to your original\nfile, the previous contents of that file have been destroyed.  Contact\nyour System Administrator BEFORE exiting if you need assistance.\n
3 %d files|%d files to edit
4 No more files|No more files to edit
5 Extra chars|Extra characters at end of "%s" command
6 %d more files|%d more files to edit
7 No write|No write since last change (:%s! overrides)
8 What?|%s: No such command from open/visual
9 What?|%s: Not an editor command
10 [Hit return to continue] 
$set 5
1 Out of memory|Out of memory - too many lines in file
2 Line overflow|Result line of join would be too long
3 %s where?|%s requires a trailing address
4 That move would do nothing!
5 Move to a moved line
6 Cannot put inside global/macro
7 Line too long|Result line after shift would be too long
8 Bad tag|Give one tag per line
9 No previous tag
10 %s: Bad tags file entry
11 No write|No write since last change (:tag! overrides)
12 No tags file
13 %s: No such tag|%s: No such tag in tags file
14 Can't yank inside global/macro
15 \nAt EOF
16 At EOF
17 Hit BOTTOM
18 Hit TOP
19 Can't undo in global|Can't undo in global commands
20 Nothing to undo
21 Nothing changed|Last undoable command didn't change anything
22 Missing lhs
23 Missing rhs
24 Missing rhs
25 No tail recursion
26 Too dangerous to map that
27 No recursive mapping
28 Missing lhs
29 Not mapped|That macro wasn't mapped
30 Too many macros
31 Too much macro text
$set 6
1 Input line too long
$set 7
1 No file|No current filename
2  [Read only]
3  [Not edited]
4  [Modified]
5 No file 
6  line %d of %d --%ld%%--
7 Pattern too long
8 Argument buffer overflow
9 No alternate filename|No alternate filename to substitute for #
10 No current filename|No current filename to substitute for %%
11 Can't make pipe to glob
12 Can't fork to do glob
13 Arg list too long
14 Arg list too long
15 No match
16 Missing filename
17 Ambiguous|Too many file names
18 Filename too long
19  [New file]
20  Block special file
21  Teletype
22  Character special file
23  Directory
24  Executable
25  Relocatable file
26  Archive
27  [Read only]
28 Write forms are 'w' and 'w>>'
29 No file|No current filename
30  File exists| File exists - use "w! %s" to overwrite
31  File is read only
32  File is read only
33  Use "w!" to write partial buffer
34  [New file]
35  [Existing file]
36  [Incomplete last line]
37  Line too long
38 Too many nested sources
39  %d lines, %D characters
40  (%D null)
101  File exists| File exists - use "wr or er %s" to overwrite
102  Use "wr" to write partial buffer
$set 8
1 Global within global|Global within global not allowed
2 Global needs re|Missing regular expression for global
3 Global command too long
4 substitution loop
5 Fail|Substitute pattern match failed
6 Substitute needs re|Missing regular expression for substitute
7 No previous re|No previous regular expression
8 No previous substitute re|No previous substitute to repeat
9 Replacement pattern too long|Replacement pattern too long - limit 256 characters
10 Line overflow|Line overflow in substitute
11 %d subs|%d substitutions
12  on %d lines
13 Regular expressions cannot be delimited by letters or digits
14 No previous scan re|No previous scanning regular expression
15 No previous substitute re|No previous substitute regular expression
16 Badly formed re|Regular expression \\ must be followed by / or ?
17 No previous re|No previous regular expression
19 Re too complex|Regular expression too complicated
20 Unmatched \\( or extra \\)|Unequal numbers of \\('s and \\)'s in regular expression
21 Awash in \\('s!|Too many \\('d subexressions in a regular expression
22 Bad collation element|A non-collating element specified where one is required
25 Bad range endpoint|First endpoint > second or ctype class used as endpoint
26 Bad character class|Invalid ctype character class '[: :]' named
27 Bad match list|Empty or incomplete match list: '[]' or '[^]'
29 No newlines in re's|Can't escape newlines into regular expressions
30 Trailing \\|Can't have a trailing \\ in a regular expression
32 RE syntax error|Invalid regular expression syntax used
$set 9
1 %s: No such option|%s: No such option - 'set all' gives all option values
2 Option %s is not a toggle
3 Missing =|Missing = in assignment to option %s
4 Digits required|Digits required after =
5 String too long|String too long in option assignment
6 Can't change type of terminal from within open/visual
$set 10
1 %d lines
2 Nonzero address required|Nonzero address required on this command
3 No lines|No lines in the buffer
4 %d more lines|%d more lines in file after %s
5 %d fewer lines|%d fewer lines in file after %s
6 Out of memory|Out of memory saving lines for undo - try using ed
44 System error %d
45 emt trap, _ovno is %d |emt trap, _ovno is %d   - try again
46 \r\nYour file has been preserved\r\n
47 \nInterrupt
$set 11
1  Tmp file too large
2  Tmp file too large
3 Out of register space (ugh)
4 Nothing in register %c
5 Can't put partial line inside macro
6 Nothing in register %c
7 Register too long|Register too long to fit in memory
$set 12
1 %s: Unknown terminal type
$set 13
1 Incomplete shell escape command|Incomplete shell escape command - use 'shell' to get a shell
2 Command too long
3 No previous command|No previous command to substitute for !
4 No alternate filename|No alternate filename to substitute for #
5 No filename|No filename to substitute for %%
6 [No write]|[No write since last change]
7 No previous command|No previous command to repeat
8 Can't make pipe for filter
9 No more processes
10 No %s!\n
11 Can't make pipe
12 No more processes
13  Can't make pipe for recovery
14  Can't fork to execute recovery
15  No recovery routine
16 %d: terminated with signal %d
17  -- core dumped
$set 14
1 Fail|Pattern not found on addressed line
2 Recursive open/visual not allowed
3 I don't know what kind of terminal you are on - all I have is '%s'.
4 [Using open mode]
5 Visual needs addressible cursor or upline capability
6 Can't use visual on a terminal which overstrikes
7 Visual requires clear screen capability
8 Visual requires scrolling
9 Screen too large for internal buffer
10 Don't know enough about your terminal to use %s
11 Terminal too wide
12 Screen too large
$set 15
1 Internal error: vscroll
2 No lines in buffer
3 Internal error: vredraw
$set 16
1 Input read error
2 %d more line
3 %d fewer line
4 %d line
5 Macro too long|Macro too long  - maybe recursive?
$set 17
1 Infinite macro loop
$set 18
1 Infinite macro loop
2 Line too long
3 Line too long
$set 19
1 Internal error: vclreol
2 Internal error: vgoto
3 Line too long for open
4 Line too long
$set 20
1 (null pointer)
