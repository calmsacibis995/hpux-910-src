# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/diff3/makefile,v $
# $Revision: 64.2.1.5 $

# Common makefile for diff3 and diff3prog

default: all

all: diff3 diff3prog

diff3:
	cp diff3.sh diff3

diff3prog:
	$(CC) $(CFLAGS) $(LDFLAGS) -o diff3prog diff3prog.c

clean:
	/bin/rm -f *.o core

clobber: clean
	/bin/rm -f diff3

# copy /usr/bin/diff3 to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/diff3
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	diff3
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp diff3 ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/diff3
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/diff3 to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/diff3
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp diff3 ../../../../Filesets.sl/CMDS-AUX/usr/bin/diff3
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/diff3
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/diff3 to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/diff3
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! cp diff3prog ../../../../Filesets.sl/CMDS-AUX/usr/lib/diff3prog
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/diff3prog
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
