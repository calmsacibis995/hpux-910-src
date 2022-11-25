default:	pwget.c
	 $(CC) -lcurses pwget.c -o pwget
	 findmsg pwget.c > pwget.msg
	 gencat pwget.cat pwget.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f pwget

# copy /bin/pwget to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/pwget
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	pwget
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp pwget ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/pwget
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
# copy /bin/pwget to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/pwget
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp pwget ../../../Filesets.sl/CMDS-AUX/bin/pwget
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/pwget
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/pwget to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/pwget
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! ln ../../../Filesets.sl/CMDS-AUX/bin/pwget ../../../Filesets.sl/CMDS-AUX/bin/grget
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
