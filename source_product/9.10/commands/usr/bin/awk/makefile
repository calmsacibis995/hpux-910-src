# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/awk/makefile,v $
# $Revision: 72.6 $
# $Release$
# $Header: makefile,v 72.6 94/08/02 14:17:52 root Exp $

# Copyright (c) 1984, 19885, 1986, 1987 AT&T
# 	All Rights Reserved
# 
# THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.
# 
# The copyright notice above does not evidence any
# actual or intended publication of such source code.


YFLAGS = -d
LIBS = -lm

FINDMSG = /usr/bin/findmsg
GENCAT = /usr/bin/gencat

OFILES = b.o main.o parse.o proctab.o tran.o lib.o run.o hpux_rel.o awk.lx.o
CFILES = b.c main.c parse.c proctab.c tran.c lib.c run.c hpux_rel.c awk.lx.c\
	 awk.g.c awk.lx.c
SOURCE = awk.h awk.g.y awk.lx.l b.c main.c maketab.c parse.c lib.c run.c tran.c hpux_rel.c awk.lx.c

default: awk.g.o awk.lx.o $(OFILES) awk.h awk.cat
	$(CC) $(CFLAGS) $(LDFLAGS) awk.g.o awk.lx.o $(OFILES) -o awk $(LIBS)
	$(CC) -Wl,-a,archive $(CFLAGS) $(LDFLAGS) awk.g.o awk.lx.o $(OFILES) -o awk.arch $(LIBS)

metrics: awk.M

awk.M:	$(CFILES)
	$(METRICS) $(CFLAGS) $(CFILES) -o awk.M $(LIBS)

$(OFILES):	awk.h prevy.tab.h 

awk.lx.c:	awk.lx.l awk.h prevy.tab.h
	lex awk.lx.l
	sed '/typedef.*uchar/d' lex.yy.c > awk.lx.c
	rm -f lex.yy.c

awk.g.o:	awk.h

y.tab.h:	awk.g.o

prevy.tab.h:	y.tab.h
	-cmp -s y.tab.h prevy.tab.h || (cp y.tab.h prevy.tab.h; echo change maketab)

proctab.c:	maketab
	./maketab >proctab.c

maketab:	prevy.tab.h maketab.c
	cc -Wl,-a,archive maketab.c -o maketab

awk.cat: awk.msg
	$(GENCAT) awk.cat awk.msg

awk.msg:
	$(FINDMSG) -a *.c > awk.msg

clean:
	/bin/rm -f *.o y.tab.[ch] prevy.tab.h maketab proctab.c awk.msg awk.lx.c

clobber: clean
	/bin/rm -f awk awk.cat awk.arch awk.M

# copy /usr/bin/awk to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/awk
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	awk
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/bin
	! cp awk ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin/awk
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp awk.cat ../../../../Filesets.sl/UX-CORE/usr/lib/nls/C/awk.cat
	! chmog 444 bin bin ../../../../Filesets.sl/UX-CORE/usr/lib/nls/C/awk.cat
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/lib/nls
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE
	! chmog 555 bin bin ../../../../Filesets.sl

