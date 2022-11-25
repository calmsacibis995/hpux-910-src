default:	banner.c
		$(CC) banner.c -o banner
		findmsg banner.c > banner.msg
		gencat banner.cat banner.msg

clean:
		rm -f *.o *.msg *.cat

clobber:	clean
		rm -f banner

# copy /usr/bin/banner to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/banner
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	banner
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp banner ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/banner
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/banner to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/banner
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp banner ../../../../Filesets.sl/CMDS-AUX/usr/bin/banner
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/banner
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
