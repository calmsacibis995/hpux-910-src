default:	uuencode.c
	 $(CC)  uuencode.c -o uuencode
	 findmsg uuencode.c > uuencode.msg
	 gencat uuencode.cat uuencode.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f uuencode

# copy /usr/bin/uuencode to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/uuencode
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	uuencode
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp uuencode ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/uuencode
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/uuencode to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/uuencode
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp uuencode ../../../../Filesets.sl/CMDS-AUX/usr/bin/uuencode
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/uuencode
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
