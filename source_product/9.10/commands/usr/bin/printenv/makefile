default:	printenv.c
	 $(CC)  printenv.c -o printenv
	 findmsg printenv.c > printenv.msg
	 gencat printenv.cat printenv.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f printenv

# copy /usr/bin/printenv to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/printenv
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	printenv
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp printenv ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/printenv
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/printenv to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/printenv
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp printenv ../../../../Filesets.sl/CMDS-AUX/usr/bin/printenv
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/printenv
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
