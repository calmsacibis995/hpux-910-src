default:	unexpand.c
	 $(CC)  unexpand.c -o unexpand
	 findmsg unexpand.c > unexpand.msg
	 gencat unexpand.cat unexpand.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f unexpand

# copy /usr/bin/unexpand to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/unexpand
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	unexpand
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp unexpand ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/unexpand
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/unexpand to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/unexpand
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp unexpand ../../../../Filesets.sl/CMDS-AUX/usr/bin/unexpand
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/unexpand
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/unexpand to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/unexpand
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp unexpand.cat ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/unexpand.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/unexpand.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
