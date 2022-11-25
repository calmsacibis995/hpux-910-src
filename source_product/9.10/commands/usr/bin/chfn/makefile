default:	chfn.c
	 $(CC) -DHP chfn.c -o chfn
	 findmsg chfn.c > chfn.msg
	 gencat chfn.cat chfn.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f chfn

# copy /usr/bin/chfn to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/chfn
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.

install:	chfn
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp chfn ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 04555 root bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/chfn
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/chfn to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/chfn
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp chfn ../../../../Filesets.sl/CMDS-AUX/usr/bin/chfn
	! chmog 04555 root bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/chfn
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
