default:	rmnl.c
	 $(CC)  rmnl.c -o rmnl
	 findmsg rmnl.c > rmnl.msg
	 gencat rmnl.cat rmnl.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f rmnl

# copy /usr/bin/rmnl to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/rmnl
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	rmnl
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp rmnl ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/rmnl
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/rmnl to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/rmnl
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp rmnl ../../../../Filesets.sl/CMDS-AUX/usr/bin/rmnl
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/rmnl
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
