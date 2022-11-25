default:	users.c
	 $(CC)  users.c -o users
	 findmsg users.c > users.msg
	 gencat users.cat users.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f users

# copy /usr/bin/users to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/users
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	users
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp users ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/users
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/users to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/users
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp users ../../../../Filesets.sl/CMDS-AUX/usr/bin/users
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/users
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
