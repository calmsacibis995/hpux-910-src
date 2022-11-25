default:	xargs.c
	 $(CC)  xargs.c -o xargs
	 findmsg xargs.c > xargs.msg
	 gencat xargs.cat xargs.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f xargs

# copy /usr/bin/xargs to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/xargs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	xargs
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp xargs ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/xargs
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/xargs to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/xargs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp xargs ../../../../Filesets.sl/CMDS-AUX/usr/bin/xargs
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/xargs
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
