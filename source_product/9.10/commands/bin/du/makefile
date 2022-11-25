default:	du.c
	 $(CC)  du.c -o du
	 findmsg du.c > du.msg
	 gencat du.cat du.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f du

# copy /bin/du to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/du
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	du
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp du ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/du
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /bin/du to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/du
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp du ../../../Filesets.sl/CMDS-MIN/bin/du
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/du
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
