default:	tabs.c
	 $(CC) tabs.c -o tabs -lcurses
	 findmsg tabs.c > tabs.msg
	 gencat tabs.cat tabs.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f tabs

# copy /usr/bin/tabs to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/tabs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	tabs
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp tabs ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/tabs
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/tabs to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/tabs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp tabs ../../../../Filesets.sl/CMDS-MIN/usr/bin/tabs
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/tabs
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/tabs to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/tabs
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp tabs.cat ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/tabs.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/tabs.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
