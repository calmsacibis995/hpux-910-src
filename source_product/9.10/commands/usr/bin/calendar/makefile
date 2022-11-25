default:	calendar.sh
		/bin/cp calendar.sh calendar

# copy /usr/bin/calendar to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/calendar
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	calendar
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp calendar ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/calendar
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/calendar to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/calendar
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp calendar ../../../../Filesets.sl/CMDS-AUX/usr/bin/calendar
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/calendar
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
