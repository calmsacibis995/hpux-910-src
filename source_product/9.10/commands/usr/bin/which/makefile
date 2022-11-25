default:	which.sh
	 /bin/cp which.sh which
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f which.sh

# copy /usr/bin/which to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/which
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	which
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp which ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/which
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/which to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/which
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp which ../../../../Filesets.sl/CMDS-MIN/usr/bin/which
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/which
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
