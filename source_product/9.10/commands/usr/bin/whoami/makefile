default:	whoami.c
	 $(CC)  whoami.c -o whoami
	 findmsg whoami.c > whoami.msg
	 gencat whoami.cat whoami.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f whoami

# copy /usr/bin/whoami to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/whoami
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	whoami
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp whoami ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/whoami
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/whoami to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/whoami
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp whoami ../../../../Filesets.sl/CMDS-MIN/usr/bin/whoami
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/whoami
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
