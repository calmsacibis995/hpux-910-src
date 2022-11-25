default:	bdf.c
		$(CC) bdf.c -o bdf
		findmsg bdf.c > bdf.msg
		gencat bdf.cat bdf.msg

clean:
		rm -f *.o *.msg *.cat

clobber:	clean
		rm -f bdf

# copy /usr/bin/bdf to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/bdf
# to 02555, bin, and sys
# change mode, owner, and group on all the parent
# directories.

install:	bdf
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp bdf ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 02555 bin sys ../../../../Filesets.sl/CMDS-AUX/usr/bin/bdf
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/bdf to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/bdf
# to 02555, bin, and sys
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp bdf ../../../../Filesets.sl/CMDS-AUX/usr/bin/bdf
	! chmog 02555 bin sys ../../../../Filesets.sl/CMDS-AUX/usr/bin/bdf
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
