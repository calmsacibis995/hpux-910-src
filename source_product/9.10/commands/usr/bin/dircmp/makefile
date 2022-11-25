default:	dircmp.sh
	 /bin/cp dircmp.sh dircmp
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f dircmp.sh

# copy /usr/bin/dircmp to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/dircmp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	dircmp
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp dircmp ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/dircmp
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/dircmp to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/dircmp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp dircmp ../../../../Filesets.sl/CMDS-AUX/usr/bin/dircmp
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/dircmp
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
