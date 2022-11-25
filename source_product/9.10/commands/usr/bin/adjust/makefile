default:	adjust.c
		$(CC) adjust.c -o adjust
		findmsg adjust.c > adjust.msg
		gencat adjust.cat adjust.msg

clean:
		rm -f *.o *.msg *.cat

clobber:	clean
		rm -f adjust

# copy /usr/bin/adjust to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/adjust
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	adjust
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp adjust ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/adjust
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/adjust to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/adjust
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp adjust ../../../../Filesets.sl/CMDS-AUX/usr/bin/adjust
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/adjust
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
