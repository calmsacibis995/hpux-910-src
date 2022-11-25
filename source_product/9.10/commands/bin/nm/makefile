default:	nm.c
	 $(CC)  nm.c -o nm
	 findmsg nm.c > nm.msg
	 gencat nm.cat nm.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f nm

# copy /bin/nm to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/nm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	nm
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp nm ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/nm
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /bin/nm to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/nm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp nm ../../../Filesets.sl/CMDS-MIN/bin/nm
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/nm
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
