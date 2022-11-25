# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/compact/makefile,v $
# $Revision: 66.1.1.6 $

# Common makefile for compact and uncompact

default: all

all: compact uncompact

compact: hpux_rel.o compact.o tree.o
	$(CC) $(CFLAGS) $(LDFLAGS) \
	    hpux_rel.o compact.o tree.o -o compact -lBUILD

uncompact: hpux_rel.o uncompact.o tree.o
	$(CC) $(CFLAGS) $(LDFLAGS) \
	    hpux_rel.o uncompact.o tree.o -o uncompact -lBUILD

compact.o uncompact.o tree.o: compact.h

clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f compact uncompact

# copy /usr/bin/compact to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/compact
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	compact
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp compact ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/compact
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/compact to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/compact
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp compact ../../../../Filesets.sl/CMDS-AUX/usr/bin/compact
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/compact
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/compact to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/compact
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp uncompact ../../../../Filesets.sl/CMDS-AUX/usr/bin/uncompact
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/uncompact
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/compact to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/compact
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp ccat.sh ../../../../Filesets.sl/CMDS-AUX/usr/bin/ccat
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/ccat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
