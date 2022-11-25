default:	paste.c
	 $(CC)  paste.c -o paste
	 findmsg paste.c > paste.msg
	 gencat paste.cat paste.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f paste

# copy /usr/bin/paste to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/paste
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	paste
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp paste ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/paste
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/paste to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/paste
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp paste ../../../../Filesets.sl/CMDS-AUX/usr/bin/paste
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/paste
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
