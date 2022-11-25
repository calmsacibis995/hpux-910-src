default:	nl.c
	 $(CC)  nl.c -o nl
	 findmsg nl.c > nl.msg
	 gencat nl.cat nl.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f nl

# copy /usr/bin/nl to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/nl
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	nl
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp nl ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/nl
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/nl to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/nl
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp nl ../../../../Filesets.sl/CMDS-AUX/usr/bin/nl
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/nl
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
