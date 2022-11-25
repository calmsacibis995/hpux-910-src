default:	leave.c
	 $(CC)  leave.c -o leave
	 findmsg leave.c > leave.msg
	 gencat leave.cat leave.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f leave

# copy /usr/bin/leave to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/leave
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	leave
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp leave ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/leave
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/leave to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/leave
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp leave ../../../../Filesets.sl/CMDS-AUX/usr/bin/leave
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/leave
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
