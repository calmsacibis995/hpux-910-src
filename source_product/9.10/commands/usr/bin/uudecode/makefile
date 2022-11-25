default:	uudecode.c
	 $(CC)  uudecode.c -o uudecode
	 findmsg uudecode.c > uudecode.msg
	 gencat uudecode.cat uudecode.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f uudecode

# copy /usr/bin/uudecode to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/uudecode
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	uudecode
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp uudecode ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/uudecode
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/uudecode to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/uudecode
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp uudecode ../../../../Filesets.sl/CMDS-AUX/usr/bin/uudecode
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/uudecode
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
