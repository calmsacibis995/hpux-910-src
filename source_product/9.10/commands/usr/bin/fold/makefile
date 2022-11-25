default:	fold.c
	 $(CC)  fold.c -o fold
	 findmsg fold.c > fold.msg
	 gencat fold.cat fold.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f fold

# copy /usr/bin/fold to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/fold
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	fold
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp fold ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/fold
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/fold to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/fold
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp fold ../../../../Filesets.sl/CMDS-AUX/usr/bin/fold
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/fold
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/fold to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/fold
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp fold.cat ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/fold.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/fold.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
