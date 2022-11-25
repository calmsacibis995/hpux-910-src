default:	chsh.c
	 $(CC)  chsh.c -o chsh
	 findmsg chsh.c > chsh.msg
	 gencat chsh.cat chsh.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f chsh

# copy /usr/bin/chsh to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/chsh
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.

install:	chsh
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp chsh ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 04555 root bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/chsh
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/chsh to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/chsh
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp chsh ../../../../Filesets.sl/CMDS-AUX/usr/bin/chsh
	! chmog 04555 root bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/chsh
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
