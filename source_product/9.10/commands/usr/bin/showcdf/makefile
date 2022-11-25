default:	showcdf.c
	 $(CC)  showcdf.c -o showcdf
	 findmsg showcdf.c > showcdf.msg
	 gencat showcdf.cat showcdf.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f showcdf

# copy /usr/bin/showcdf to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/showcdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	showcdf
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp showcdf ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/showcdf
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/showcdf to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/showcdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp showcdf ../../../../Filesets.sl/CMDS-AUX/usr/bin/showcdf
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/showcdf
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
