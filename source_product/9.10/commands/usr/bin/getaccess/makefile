default:	getaccess.c
	 $(CC)  getaccess.c -o getaccess
	 findmsg getaccess.c > getaccess.msg
	 gencat getaccess.cat getaccess.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f getaccess

# copy /usr/bin/getaccess to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/getaccess
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	getaccess
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp getaccess ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/getaccess
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/getaccess to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/getaccess
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp getaccess ../../../../Filesets.sl/CMDS-AUX/usr/bin/getaccess
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/getaccess
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/getaccess to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/getaccess
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp getaccess.cat ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/getaccess.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/getaccess.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
