# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/dos/makefile,v $
# $Revision: 64.3.1.10 $

# Common makefile for dos/hpux utilities.  These are:
#
# 	doschmod
#	doscp
#	dosdf
#	dosll
# 	dosls
#	dosmkdir
# 	dosrm
#	dosrmdir
# 	dos2ux
#	ux2dos

# dos2ux and ux2dos are both created from convert.c with a flag
# (-DTODOS) controlling the action. Therefore, convert.o MUST
# always be deleted BEFORE making dos2ux or ux2dos.


#	Use the following flags when building debug versions of the
#	DOS utilities.
# FLAGS= -g -DDEBUG


INCLUDES=dos.h 
LIBS=dos1.o dos.o dosmain.o

PROGS=dosrm.o dosls.o dosdf.o doscp.o doschmod.o dosmkdir.o
CMDS=dosrm dosls dosdf doscp doschmod dosmkdir dosll  dosrmdir dos2ux \
     ux2dos

$(LIBS): $(INCLUDES)
$(PROGS): $(INCLUDES)

default: all

all:	$(CMDS)


doslib:	$(LIBS) $(INCLUDES) $(PROGS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) $(PROGS) -o doslib

doscp:	doslib
	/bin/rm -f doscp
	ln doslib doscp

doschmod: doslib
	/bin/rm -f doschmod
	ln doslib doschmod

dosmkdir: doslib
	/bin/rm -f dosmkdir
	ln doslib dosmkdir

dosls:	doslib
	/bin/rm -f dosls
	ln doslib dosls

dosrmdir: doslib
	/bin/rm -f dosrmdir
	ln doslib dosrmdir

dosrm:	doslib
	/bin/rm -f dosrm
	ln doslib dosrm

dosll:	doslib
	/bin/rm -f dosll
	ln doslib dosll

dosdf:  doslib
	/bin/rm -f dosdf
	ln doslib dosdf

ux2dos: dconvert.o
	$(CC) $(LDFLAGS) -o ux2dos convert.o
	/bin/rm -f convert.o

dos2ux: convert.o
	$(CC) $(LDFLAGS) -o dos2ux convert.o
	/bin/rm -f convert.o

convert.o: convert.c
	$(CC) $(CFLAGS) -c convert.c

dconvert.o: convert.c
	$(CC) $(CFLAGS) -DTODOS -c convert.c

clean:
	/bin/rm -f *.o

clobber:	clean
	/bin/rm -f $(CMDS) dosln dosmv
# copy /usr/bin/dos to the . relative fileset DOS-UTILS
# change mode, owner, and group on /usr/bin/dos
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! cp dos2ux ../../../../Filesets.sl/DOS-UTILS/usr/bin/dos2ux
	! chmog 0555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/bin/dos2ux
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/dos to the . relative fileset DOS-UTILS
# change mode, owner, and group on /usr/bin/dos
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! cp doschmod ../../../../Filesets.sl/DOS-UTILS/usr/bin/doschmod
	! chmog 0555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/bin/doschmod
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/dos to the . relative fileset DOS-UTILS
# change mode, owner, and group on /usr/bin/dos
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! cp ux2dos ../../../../Filesets.sl/DOS-UTILS/usr/bin/ux2dos
	! chmog 0555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/bin/ux2dos
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/dos to the . relative fileset DOS-UTILS
# change mode, owner, and group on /usr/bin/dos
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! ln ../../../../Filesets.sl/DOS-UTILS/usr/bin/doschmod ../../../../Filesets.sl/DOS-UTILS/usr/bin/doscp
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/dos to the . relative fileset DOS-UTILS
# change mode, owner, and group on /usr/bin/dos
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! ln ../../../../Filesets.sl/DOS-UTILS/usr/bin/doschmod ../../../../Filesets.sl/DOS-UTILS/usr/bin/dosdf
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/dos to the . relative fileset DOS-UTILS
# change mode, owner, and group on /usr/bin/dos
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! ln ../../../../Filesets.sl/DOS-UTILS/usr/bin/doschmod ../../../../Filesets.sl/DOS-UTILS/usr/bin/dosll
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/dos to the . relative fileset DOS-UTILS
# change mode, owner, and group on /usr/bin/dos
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! ln ../../../../Filesets.sl/DOS-UTILS/usr/bin/doschmod ../../../../Filesets.sl/DOS-UTILS/usr/bin/dosls
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/dos to the . relative fileset DOS-UTILS
# change mode, owner, and group on /usr/bin/dos
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! ln ../../../../Filesets.sl/DOS-UTILS/usr/bin/doschmod ../../../../Filesets.sl/DOS-UTILS/usr/bin/dosmkdir
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/dos to the . relative fileset DOS-UTILS
# change mode, owner, and group on /usr/bin/dos
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! ln ../../../../Filesets.sl/DOS-UTILS/usr/bin/doschmod ../../../../Filesets.sl/DOS-UTILS/usr/bin/dosrm
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/dos to the . relative fileset DOS-UTILS
# change mode, owner, and group on /usr/bin/dos
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! ln ../../../../Filesets.sl/DOS-UTILS/usr/bin/doschmod ../../../../Filesets.sl/DOS-UTILS/usr/bin/dosrmdir
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DOS-UTILS/
	! chmog 555 bin bin ../../../../Filesets.sl/
