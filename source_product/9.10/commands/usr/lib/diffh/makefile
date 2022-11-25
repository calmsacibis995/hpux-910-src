default:	diffh.c
	 $(CC)  diffh.c -o diffh
	 findmsg diffh.c > diffh.msg
	 gencat diffh.cat diffh.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f diffh

# copy /usr/lib/diffh to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/lib/diffh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	diffh
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! cp diffh ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/diffh
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/lib/diffh to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/lib/diffh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! cp diffh ../../../../Filesets.sl/CMDS-AUX/usr/lib/diffh
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/diffh
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/diffh to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/lib/diffh
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp diffh.cat ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/diffh.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/diffh.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
