default:	pathchk.c
	 $(CC)  pathchk.c -o pathchk
	 findmsg pathchk.c > pathchk.msg
	 gencat pathchk.cat pathchk.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f pathchk

# copy /usr/bin/pathchk to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/pathchk
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	pathchk
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp pathchk ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/pathchk
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/pathchk to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/pathchk
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp pathchk ../../../../Filesets.sl/CMDS-MIN/usr/bin/pathchk
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/pathchk
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/pathchk to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/pathchk
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp pathchk.cat ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/pathchk.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/pathchk.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
