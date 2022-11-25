# @(#) $Revision: 66.2.1.8 $
#
# common build makefile for ied
#

OBJS		= ied.o emacs.o history.o vi.o edit.o
SRCS		= ied.c edit.c emacs.c history.c vi.c hpux_rel.c
HDRS		= edit.h def.h history.h terminal.h
MISC		= makefile
DELIVERABLES	= ied ied.1 README README.keymap dvorak.layout \
		  table.dvorak table.identity

default: ied

ied: version.o $(OBJS)
	$(CC) $(LFLAGS) version.o $(OBJS) -o $@

version.c: hpux_rel.c
	sed -e '/e_version/s/$$Revision: //' \
	    -e '/e_version/s/ $$ \$$Date:/ /' \
	    -e '/e_version/s/ ..:..:.. $$"/"/' <hpux_rel.c >version.c

deliver: ied
	shar $(DELIVERABLES) > ied.shar

source:
	shar $(DELIVERABLES) $(SRCS) $(HDRS) $(MISC) > ied.source.shar

emacs.o: defs.h edit.h history.h

history.o: defs.h history.h

vi.o: defs.h edit.h history.h terminal.h

edit.o: defs.h edit.h history.h terminal.h

ied.o: defs.h edit.h

clean:
	rm -f *.o

clobber: clean
	rm -f ied ied.shar ied.source.shar

# copy /usr/bin/ied to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/ied
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	ied
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp ied ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/ied
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/ied to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/ied
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp ied ../../../../Filesets.sl/CMDS-AUX/usr/bin/ied
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/ied
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/ied to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/ied
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/ied
	! cp README.keymap ../../../../Filesets.sl/CMDS-AUX/usr/lib/ied/README.keymap
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/ied/README.keymap
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/ied
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/ied to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/ied
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/ied
	! cp dvorak.layout ../../../../Filesets.sl/CMDS-AUX/usr/lib/ied/dvorak.layout
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/ied/dvorak.layout
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/ied
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/ied to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/ied
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/ied
	! cp table.dvorak ../../../../Filesets.sl/CMDS-AUX/usr/lib/ied/table.dvorak
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/ied/table.dvorak
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/ied
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/ied to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/ied
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/ied
	! cp table.identity ../../../../Filesets.sl/CMDS-AUX/usr/lib/ied/table.identity
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/ied/table.identity
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/ied
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
