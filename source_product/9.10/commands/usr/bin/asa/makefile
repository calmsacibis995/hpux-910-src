default:	asa.c
		$(CC) asa.c -o asa
		findmsg asa.c > asa.msg
		gencat asa.cat asa.msg

clean:
		rm -f *.o *.msg *.cat

clobber:	clean
		rm -f asa

# copy /usr/bin/asa to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/asa
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	asa
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp asa ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/asa
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/asa to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/asa
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp asa ../../../../Filesets.sl/CMDS-AUX/usr/bin/asa
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/asa
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
