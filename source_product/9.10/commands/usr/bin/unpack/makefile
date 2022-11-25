default:	unpack.c
	 $(CC)  unpack.c -o unpack
	 findmsg unpack.c > unpack.msg
	 gencat unpack.cat unpack.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f unpack

# copy /usr/bin/unpack to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/unpack
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	unpack
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp unpack ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/unpack
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/unpack to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/unpack
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp unpack ../../../../Filesets.sl/CMDS-AUX/usr/bin/unpack
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/unpack
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/unpack to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/unpack
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! ln ../../../../Filesets.sl/CMDS-AUX/usr/bin/unpack ../../../../Filesets.sl/CMDS-AUX/usr/bin/pcat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
