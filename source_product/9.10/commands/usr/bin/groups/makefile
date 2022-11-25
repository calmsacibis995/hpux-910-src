default:	groups.c
	 $(CC)  groups.c -o groups
	 findmsg groups.c > groups.msg
	 gencat groups.cat groups.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f groups

# copy /usr/bin/groups to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/groups
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	groups
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp groups ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/groups
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/groups to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/groups
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp groups ../../../../Filesets.sl/CMDS-AUX/usr/bin/groups
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/groups
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
