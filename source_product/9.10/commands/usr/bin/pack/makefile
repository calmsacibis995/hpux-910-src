default:	pack.c
	 $(CC)  pack.c -o pack
	 findmsg pack.c > pack.msg
	 gencat pack.cat pack.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f pack

# copy /usr/bin/pack to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/pack
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	pack
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp pack ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/pack
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/pack to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/pack
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp pack ../../../../Filesets.sl/CMDS-AUX/usr/bin/pack
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/pack
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
