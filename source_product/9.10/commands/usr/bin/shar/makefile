default:	shar.c
	 $(CC)  shar.c -o shar
	 findmsg shar.c > shar.msg
	 gencat shar.cat shar.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f shar

# copy /usr/bin/shar to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/shar
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	shar
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp shar ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/shar
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/shar to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/shar
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp shar ../../../../Filesets.sl/CMDS-AUX/usr/bin/shar
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/shar
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
