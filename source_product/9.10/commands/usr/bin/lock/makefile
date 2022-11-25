default:	lock.c
	 $(CC)  lock.c -o lock
	 findmsg lock.c > lock.msg
	 gencat lock.cat lock.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f lock

# copy /usr/bin/lock to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/lock
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	lock
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp lock ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/lock
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/lock to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/lock
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp lock ../../../../Filesets.sl/CMDS-AUX/usr/bin/lock
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/lock
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
