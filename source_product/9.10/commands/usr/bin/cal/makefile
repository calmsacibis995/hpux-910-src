default:	cal.c
		$(CC) cal.c -o cal
		findmsg cal.c > cal.msg
		gencat cal.cat cal.msg

clean:
		rm -f *.o *.msg *.cat

clobber:	clean
		rm -f cal

# copy /usr/bin/cal to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/cal
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cal
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp cal ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/cal
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/cal to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/cal
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp cal ../../../../Filesets.sl/CMDS-AUX/usr/bin/cal
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/cal
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
