default:	sdiff.c
	 $(CC)  sdiff.c -o sdiff
	 findmsg sdiff.c > sdiff.msg
	 gencat sdiff.cat sdiff.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f sdiff

# copy /usr/bin/sdiff to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/sdiff
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	sdiff
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp sdiff ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/sdiff
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/sdiff to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/sdiff
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp sdiff ../../../../Filesets.sl/CMDS-AUX/usr/bin/sdiff
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/sdiff
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
