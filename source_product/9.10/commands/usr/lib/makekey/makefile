default:	makekey.c
	 $(CC)  makekey.c -o makekey
	 findmsg makekey.c > makekey.msg
	 gencat makekey.cat makekey.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f makekey

# copy /usr/lib/makekey to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/lib/makekey
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	makekey
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! cp makekey ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/makekey
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/lib/makekey to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/lib/makekey
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! cp makekey ../../../../Filesets.sl/CMDS-AUX/usr/lib/makekey
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/makekey
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
