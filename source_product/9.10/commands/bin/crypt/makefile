default:	crypt.c
	 $(CC)  crypt.c -o crypt
	 findmsg crypt.c > crypt.msg
	 gencat crypt.cat crypt.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f crypt

# copy /bin/crypt to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/crypt
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	crypt
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp crypt ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/crypt
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
# copy /bin/crypt to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/crypt
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp crypt ../../../Filesets.sl/CMDS-AUX/bin/crypt
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/crypt
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
