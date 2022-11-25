# $Header: makefile,v 72.50 94/08/02 16:43:04 root Exp $
# @(#) $Revision: 72.50 $
#
# C Shell makefile

# FLAGS =

# FLAGS:  -DTRACE_DEBUG       Used to print debug execution trace and variable
#                             trace.  Currently used in files sh.c, sh.exec.c,
#                             sh.glob.c, sh.proc.c, and sh.sem.c.
# FLAGS:  -DSPIN_CHILD_DEBUG  Used to put an infinite loop just after the
#                             fork in the child process in pfork() (sh.proc.c).
#                             Allows a csh child to be started and then be
#                             adopted by a debugger.
# FLAGS:  -DVFORK_DEBUG       Used to print debug variable trace between the
#                             fork and exec.  Currently used in doexec() 
#                             (sh.exec.c).
# FLAGS:  -DENV_DEBUG         Used to print out the environment just after a
#                             fork in the child process in pfork() (sh.proc.c).
# FLAGS:  -DFNMATCH_DEBUG     Used to print the calling parameters to fnmatch
#                             in amatch() and Gmatch () (sh.glob.c).
# FLAGS:  -DFNMATCH_DEBUG_2   Used to print the strings as integers before a
#                             call to fnmatch in Gmatch () (sh.glob.c).  This
#                             is nested inside a #ifdef FNMATCH_DEBUG
# FLAGS:  -DXSET_DEBUG        Used to print a trace of values in xset () 
#                             (sh.set.c).
# FLAGS:  -DADDPATH_DEBUG     Used to print the character getting added to
#                             the global gpath in addpath() (sh.glob.c).
# FLAGS:  -DSTRING_DEBUG      Used to print the translated string in to_char()
#                             (sh.misc.c).
# FLAGS:  -DSIGNAL_DEBUG      Used to print the signal handlers in doexec()
#                             (sh.exec.c).
# FLAGS:  -DHASH_DEBUG        Used to print the values of hits and misses for
#                             hashing in doexec() and hashstat() (sh.exec.c).
# FLAGS:  -DHASH_DEBUG_TABLE  Used to print the entire hash table as it is being
#                             built in dohash() (sh.exec.c).
# FLAGS:  -DSPIN_DEBUG        Used to put an infinite loop at the beginning of 
#                             the program in main() (sh.c).  Allows a csh to be
#                             started and then be adopted by a debugger.
# FLAGS:  -DVFORK             Used for Berkeley vfork semantics.  This uses 
#                             un-supported features so isn't usable under the 
#                             8.0 VM system.
# FLAGS:  -DDEBUG_DONEINP     Used to trace values that children may pass back
#                             to the parent in the exit status.  Currently used
#                             in execute (sh.sem.c), error (sh.err.c), exit 
#                             (sh.c), and pchild (sh.proc.c).
# FLAGS:  -DDEBUG_EXEC        Used to print the command being exec'd.  Currently
#                             used in texec (sh.exec.c).
# FLAGS:  -DDEBUG_BACKQUOTE   Used to figure out the return values from the
#                             routine that checks for backquotes in a command.
#                             Currently used in hasback (sh.sem.c).
# FLAGS:  -DDEGUG_LANG        Used to print LANG values from setenv commands.
#                             Currently used in dosetenv (sh.func.c).
# FLAGS:  -DDEBUG_PROC        Used to dump the process table.  Currently
#                             used in pchild (sh.proc.c) and palloc (sh.proc.c).
# FLAGS:  -DDEBUG_SETENV      Used to dump the arguments and various other
#                             values in setenv (sh.func.c).
# FLAGS:  -DDEBUG_SET         Used to dump the arguments in doset (sh.set.c).
#                             Also in setq (sh.set.c).
# FLAGS:  -DDEBUG_INPUT       Used to print information out about command 
#                             input to the shell.  Used in word and getC, both
#                             in sh.lex.c
# FLAGS:  -DDEBUG_READC       Used in readc (sh.lex.c) to print when EOF is 
#                             seen in the input stream, prints to a file.
# FLAGS:  -DDEBUG_BGETC       Used in begetc (sh.lex.c) to print when EOF is 
#                             seen in the input stream, prints to a file.
# FLAGS:  -DDEBUG_REWIND      Used in process (sh.c) to print the ignoreeof 
#                             variable.
# FLAGS:  -DDEBUG_BACKEVAL    Used in backeval (sh.glob.c) to print out the
#                             command being executed.
# FLAGS:  -DDEBUG_ECHO        Used in echo (sh.func.c) to print out the
#                             strings being echo'd.
# FLAGS:  -DDEBUG_FILE_LEX    Used to print lex information to a file in lex
#                             (sh.lex.c).
# FLAGS:  -DDEBUG_SIGNAL      Used to print when the signal handler for SIGINT
#                             is being set to SIG_DFL in sigset (jobs.c).
# FLAGS:  -DDEBUG_EXIT        Used to track where exit is called.  Currently
#                             used in main(), exitstat(), and exit() (sh.c),
#                             dologin() and donewgrp() (sh.func.c), error()
#                             (sh.err.c), and prexit() (getwd.c).
# FLAGS:  -DDEBUG_CHILD       Used to track results from child processes back
#                             in the parent.  Used in pchild() and pjwait()
#                             (sh.proc.c).
# FLAGS:  -DDEBUG_STATUS      Used to track the status returned from child 
#                             processes.  Used in pjwait() (sh.proc.c).
# FLAGS:  -DDEBUG_PFLUSH      Used to track when the pflush routine is called.
#                             Used in pchild(), pnote(), pjwait(), pflushall(),
#                             pflush(), and dojobs() (sh.proc.c).
# FLAGS:  -DDEBUG_PFIND       Used to track when the pfind routine is called.
#                             Used in dofg(), dofg1(), dobg(), dobg1(), pkill(),
#                             pfind(), and donotify() (sh.proc.c).
# FLAGS:  -DDEBUG_TTYECHO     Used to trace when characters are echo'd.  Used
#                             in getline() (tenex.c).
# FLAGS:  -DDEBUG_TREE        Used to dump the command tree.  Used in execute()
#                             (sh.sem.c).
# FLAGS:  -DDEBUG_IF_THEN_ELSE Used to dump procedure trace information.  Used 
#                              in doif(), doelse(), reexecute() and search() 
#                              (sh.func.c).
# FLAGS:  -DDEBUG_SEARCH      Used to trace execution of the search routine.   
#                             Used in pintr1() (sh.c), doif(), doelse(),
#                             dogoto(), doswitch(), preread(), doswbrk(),
#                             getword(), and toend() (sh.func.c).
# FLAGS:  -DDEBUG_ALIAS       Used to trace execution with aliases.  Used in
#                             alias(), asyntax(), asyn0() (sh.parse.c).
# FLAGS:  -DDEBUG_DIRS        Used to trace execution with directories.  Used in
#                             dodirs(), dochngd() (sh.dir.c).
# FLAGS:  -DDEBUG_LEX_HIST    Used to trace routine execution in sh.lex.c.

LIBS    = -lcurses
OBJS    = alloc.o getwd.o jobs.o pprintf.o sh.dir.o sh.dol.o \
	  sh.err.o sh.exec.o sh.exp.o sh.func.o sh.glob.o sh.hist.o \
	  sh.init.o sh.lex.o sh.misc.o sh.o sh.parse.o sh.print.o \
	  sh.proc.o sh.sem.o sh.set.o sh.time.o tenex.o
REL     = hpux_rel.o
REL_C   = hpux_rel.c
FINDMSG	= /usr/bin/findmsg
GENCAT	= /usr/bin/gencat
MSG1    = alloc.c getwd.c sh.c sh.dir.c sh.dol.c sh.exec.c sh.exp.c \
	  sh.func.c sh.glob.c sh.lex.c sh.misc.c sh.parse.c sh.proc.c \
	  sh.sem.c sh.set.c tenex.c
MSG2    = sh.init.c
MSG3    = sh.err.c

default: csh csh.cat

metrics: csh.M

csh: $(OBJS) $(REL) sh.local.h
	@if ifdef RFA; then					\
	    echo $(CC) $(CFLAGS) $(OBJS) $(REL)			\
		       $(LIBS) -ln $(LDFLAGS) -o csh;		\
	    $(CC) $(CFLAGS) $(OBJS) $(REL)			\
		  $(LIBS) -ln $(LDFLAGS) -o csh;		\
	else							\
	    echo $(CC) $(CFLAGS) $(OBJS) $(REL)			\
		  $(LIBS) $(LDFLAGS) -o csh;			\
	    $(CC) $(CFLAGS) $(OBJS) $(REL)			\
		  $(LIBS) $(LDFLAGS) -o csh;			\
	fi

csh.M: $(MSG1) $(MSG2) $(MSG2) $(REL_C) sh.local.h
	$(METRICS) $(CFLAGS) $(MSG1) $(MSG2) $(MSG3) $(REL_C) $(LIBS) -o csh.M

csh.cat: csh1.msg csh2.msg csh3.msg
	$(GENCAT) $@ csh1.msg
	$(GENCAT) $@ csh2.msg
	$(GENCAT) $@ csh3.msg

csh1.msg: $(MSG1)
	rm -f csh1.msg
	for i in $(MSG1); do           \
	    $(FINDMSG) $$i >>csh1.msg; \
	done

csh2.msg: $(MSG2)
	$(FINDMSG) $(MSG2) >csh2.msg

csh3.msg:  $(MSG3)
	$(FINDMSG) $(MSG3) >csh3.msg

clean:
	rm -f core *.o *.msg \#*

clobber: clean
	rm -f csh csh.cat csh.M

.c.o:
	$(CC) -c $(CFLAGS) $(FLAGS) $*.c

# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	csh
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp csh ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/csh
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp csh ../../../Filesets.sl/CMDS-MIN/bin/csh
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/csh
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp csh.cat ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/csh.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/csh.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! touch ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/alias
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/alloc
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/bg
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/case
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/cd
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/chdir
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/dirs
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/eval
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/exec
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/fg
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/foreach
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/glob
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/hashstat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/history
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/if
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/jobs
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/kill
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/login
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/logout
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/newgrp
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/nice
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/nohup
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/notify
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/onintr
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/popd
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/pushd
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/rehash
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/repeat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/set
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/setenv
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/source
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/stop
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/switch
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/time
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/umask
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/unalias
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/unhash
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/unset
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/unsetenv
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/wait
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/@ ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins/while
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/builtins
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/csh to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/csh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
