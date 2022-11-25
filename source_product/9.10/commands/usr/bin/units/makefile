# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/units/makefile,v $
# $Revision: 64.2.1.5 $

#
# Makefile for units(1)
#
# This makefile is necessary so that the version number (as per
# hpux_rel.c) is updated if unittab is changed, and so unittab gets
# named properly.
#

default: all

all: units unittab

units:	units.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o units units.c hpux_rel.c

inittab: unittab.sh
	cp unittab.sh unittab

clean:
	/bin/rm -f units.o hpux_rel.o

clobber: clean
	/bin/rm -f units unittab

# copy /usr/bin/units to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/units
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	units
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp units ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/units
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/units to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/units
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp units ../../../../Filesets.sl/CMDS-AUX/usr/bin/units
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/units
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/units to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/units
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! cp unittab.sh ../../../../Filesets.sl/CMDS-AUX/usr/lib/unittab
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/unittab
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
