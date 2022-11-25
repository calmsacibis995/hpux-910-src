default:	newform.c
	 $(CC)  newform.c -o newform
	 findmsg newform.c > newform.msg
	 gencat newform.cat newform.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f newform

# copy /usr/bin/newform to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/newform
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	newform
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp newform ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/newform
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/newform to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/newform
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp newform ../../../../Filesets.sl/CMDS-AUX/usr/bin/newform
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/newform
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
