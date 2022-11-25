default:	tr.c
	 $(CC)  tr.c -o tr
	 findmsg tr.c > tr.msg
	 gencat tr.cat tr.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f tr

# copy /usr/bin/tr to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/tr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	tr
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp tr ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/tr
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/tr to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/tr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp tr ../../../../Filesets.sl/CMDS-MIN/usr/bin/tr
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/tr
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
