# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/man/makefile,v $
# @(#) $Revision: 66.2.1.10 $

# Common makefile for man

MSGSRC	= man.msg whereis.msg
FINDMSG = /usr/bin/findmsg
GENCAT  = /usr/bin/gencat
CMDS	= man catman getNAME whereis parseman mkwhatis man.cat


default: all

all: $(CMDS)

man:	man.o manlib.o
	$(CC) $(LDFLAGS) -o man man.o manlib.o

catman: catman.o manlib.o
	$(CC) $(LDFLAGS) -o catman catman.o manlib.o

getNAME: getNAME.o
	$(CC) $(LDFLAGS) -o getNAME getNAME.o

whereis: whereis.o
	$(CC) $(LDFLAGS) -o whereis whereis.o

parseman: parseman.o
	$(CC) $(LDFLAGS) -o parseman parseman.o manlib.o

mkwhatis: mkwhatis.sh
	cp mkwhatis.sh mkwhatis

man.cat: $(MSGSRC)
	$(GENCAT) $@ $(MSGSRC)

clean:
	/bin/rm -f *.o a.out $(MSGSRC)

clobber:  clean
	/bin/rm -f  $(CMDS)

.c.msg:
	$(FINDMSG) $*.c > $*.msg

.msg.cat:
	$(GENCAT) $*.cat $*.msg

.SUFFIXES: .cat .msg

# copy /usr/bin/man to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/man
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	man
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp man ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/man
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/man to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/man
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/etc
	! cp catman ../../../../Filesets.sl/TEXT-FMT/etc/catman
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/etc/catman
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/etc
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/man to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/man
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp man ../../../../Filesets.sl/CMDS-MIN/usr/bin/man
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/man
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/man to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/man
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp whereis ../../../../Filesets.sl/CMDS-MIN/usr/bin/whereis
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/whereis
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/man to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/man
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/lib
	! cp getNAME ../../../../Filesets.sl/TEXT-FMT/usr/lib/getNAME
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/lib/getNAME
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/man to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/man
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/lib
	! cp mkwhatis ../../../../Filesets.sl/TEXT-FMT/usr/lib/mkwhatis
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/lib/mkwhatis
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/man to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/man
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp man.cat ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/man.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/man.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/man to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/man
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/lib
	! cp parseman ../../../../Filesets.sl/TEXT-FMT/usr/lib/parseman
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/lib/parseman
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
	! chmog 555 bin bin ../../../../Filesets.sl/
