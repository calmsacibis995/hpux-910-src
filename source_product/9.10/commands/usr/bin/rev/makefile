default:	rev.c
	 $(CC)  rev.c -o rev
	 findmsg rev.c > rev.msg
	 gencat rev.cat rev.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f rev

# copy /usr/bin/rev to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/rev
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	rev
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp rev ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/rev
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/rev to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/rev
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp rev ../../../../Filesets.sl/CMDS-AUX/usr/bin/rev
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/rev
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
