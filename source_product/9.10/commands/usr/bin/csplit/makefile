default:	csplit.c
	 $(CC)  csplit.c -o csplit
	 findmsg csplit.c > csplit.msg
	 gencat csplit.cat csplit.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f csplit

# copy /usr/bin/csplit to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/csplit
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	csplit
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp csplit ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/csplit
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/csplit to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/csplit
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp csplit ../../../../Filesets.sl/CMDS-AUX/usr/bin/csplit
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/csplit
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
