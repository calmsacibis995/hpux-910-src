# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/tset/makefile,v $
# $Revision: 64.2.1.5 $

# Common makefile for tset

LIBS = -lcurses

default: all

all:	tset reset

tset:	tset.c
	if [ -f tset.delays. ]; then mv tset.delays. tset.delays.h; fi
	$(CC) $(CFLAGS) $(LDFLAGS) tset.c -o $@ $(LIBS)

reset:	tset
	ln tset reset

clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f tset reset

# copy /usr/bin/tset to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/tset
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	tset
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp tset ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/tset
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/tset to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/tset
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp tset ../../../../Filesets.sl/CMDS-MIN/usr/bin/tset
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/tset
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/tset to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/tset
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! ln ../../../../Filesets.sl/CMDS-MIN/usr/bin/tset ../../../../Filesets.sl/CMDS-MIN/usr/bin/reset
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
