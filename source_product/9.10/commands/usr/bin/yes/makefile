default:	yes.c
	 $(CC)  yes.c -o yes
	 findmsg yes.c > yes.msg
	 gencat yes.cat yes.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f yes

# copy /usr/bin/yes to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/yes
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	yes
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp yes ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/yes
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/yes to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/yes
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp yes ../../../../Filesets.sl/CMDS-AUX/usr/bin/yes
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/yes
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
