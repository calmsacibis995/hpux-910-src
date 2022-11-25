default:	logname.c
	 $(CC)  logname.c -o logname
	 findmsg logname.c > logname.msg
	 gencat logname.cat logname.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f logname

# copy /usr/bin/logname to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/logname
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	logname
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp logname ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/logname
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/logname to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/logname
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp logname ../../../../Filesets.sl/CMDS-AUX/usr/bin/logname
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/logname
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/logname to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/logname
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp logname.cat ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/logname.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/logname.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
