# keyshell nmakefile
# RCS ID: @(#) $Header: Makefile,v 66.13 91/01/01 15:06:53 rpt Exp $
# Copyright (c) HP, 1989

*.o	      :	 *.c
		 $(CC) -DKEYSHELL $(CFLAGS) -c $(>)

*.c	      :	 *.y
		 yacc $(>)
		 mv y.tab.c $(<)
		 chmod =r $(<)

*	      :	 *.cpp
		 rm -f $(<)
		 /lib/cpp -A $(>) | ssp >$(<)
		 chmod =r $(<)

default	      :	 TSM keyshell.a softkeys builtins KSH88 KEYSHELL keysh.cat

KSH88	      :; cp -r ../ksh88 .
		 mv ksh88 KSH88
		 echo "# Keyshell sed(1)'d makefile" >KSH88/MAKEFILE
		 sed 's!LIBS =!LIBS = -lcurses!; s!-DKSHELL!-DKSHELL -DKEYSHELL!; s!^emacs\.o!EMACS.o!; s!^vi\.o!VI.o!; s!^hpux_rel\.o!HPUX_REL.o!; s!emacs\.o!../keyshell.a!; s!vi\.o!!; s!hpux_rel\.o!!; s!edit\.o:!edit.o: ../kshhooks.h!' <KSH88/makefile >>KSH88/MAKEFILE
		 (cd KSH88; make -f MAKEFILE clobber)

TSM	      :; mkdir -p TSM/include
		 cp ../tsm/include/facetterm.h TSM/include


KEYSHELL      :; (cd KSH88; make -f MAKEFILE)
		 cp KSH88/ksh keysh

keyshell.a    :: keyshell.c \
		 global.c \
		 translate.y \
		 intrinsics.c \
		 config.c \
		 extents.c \
		 word.c \
		 edit.c \
		 emacs.c \
		 vi.c \
		 keyboard.c \
		 display.c \
		 message.c \
		 \
		 clist.c \
		 chunk.c \
		 hierarchy.c \
		 linklist.c \
		 string2.c \
		 select.c \
		 quote.c \
		 buffer.c \
		 hpux_rel.c

softkeys      :: softkeys.cpp code.h

builtins      :: builtins.cpp code.h

keysh.cat     : message.c message.h
		 findmsg message.c >keysh.msg
		 chmod =r keysh.msg
		 gencat keysh.cat keysh.msg
		 rm -f keysh.msg

backup	      :; tar cf BACKUP/TAR`date +%d%b` Makefile bugs *rc *.[ych] *.cpp

public	      :; mv /users/rpt/demo/keysh /users/rpt/demo/busy/$$
		 ignore rm /users/rpt/demo/busy/[0-9]*
		 cp keysh /users/rpt/demo
		 chmod u+w /users/rpt/demo/builtins
		 chmod u+w /users/rpt/demo/softkeys
		 chmod u+w /users/rpt/demo/keyshrc
		 chmod u+w /users/rpt/demo/keysh.cat
		 chmod u+w /users/rpt/demo/help
		 chmod u+w /users/rpt/demo/bugs
		 chmod u+w /users/rpt/demo/keysh.1
		 chmod u+w /users/rpt/demo/softkeys.4
		 cp builtins softkeys keyshrc keysh.cat help bugs \
		   /users/rpt/demo
		 cp ../doc/keysh.1 ../doc/softkeys.4 /users/rpt/demo
                 rm -f /users/rpt/demo/keysh.tar
		 tar chf /users/rpt/demo/keysh.tar /usr/bin/keysh /usr/keysh \
		   /usr/man/man1/keysh.1 /usr/man/man4/softkeys.4 \
		   /usr/lib/nls/C/keysh.cat /usr/lib/terminfo/h/hpterm \
		   /usr/lib/terminfo/2/2392

ninstall300   :; cp /usr/bin/keysh ~/ninstall/300/usr/bin
                 cp /usr/keysh/builtins ~/ninstall/300/usr/keysh
                 cp /usr/keysh/C/[a-z]* ~/ninstall/300/usr/keysh/C
