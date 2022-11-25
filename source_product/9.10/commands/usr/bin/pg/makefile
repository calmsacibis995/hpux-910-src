default:	pg.c
	 $(CC) pg.c -o pg -lcurses
	 findmsg pg.c > pg.msg
	 gencat pg.cat pg.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f pg

# copy /usr/bin/pg to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/pg
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	pg
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp pg ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/pg
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/pg to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/pg
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp pg ../../../../Filesets.sl/CMDS-AUX/usr/bin/pg
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/pg
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/pg to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/pg
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp pg.cat ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/pg.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/pg.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
