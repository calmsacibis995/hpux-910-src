default:	split.c
	 $(CC)  split.c -o split
	 findmsg split.c > split.msg
	 gencat split.cat split.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f split

# copy /usr/bin/split to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/split
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	split
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp split ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/split
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/split to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/split
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp split ../../../../Filesets.sl/CMDS-AUX/usr/bin/split
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/split
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/split to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/split
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp split.cat ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/split.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/split.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
