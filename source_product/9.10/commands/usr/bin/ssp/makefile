default:	ssp.c
	 $(CC)  ssp.c -o ssp
	 findmsg ssp.c > ssp.msg
	 gencat ssp.cat ssp.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f ssp

# copy /usr/bin/ssp to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/ssp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	ssp
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp ssp ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/ssp
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/ssp to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/ssp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp ssp ../../../../Filesets.sl/CMDS-AUX/usr/bin/ssp
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/ssp
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
