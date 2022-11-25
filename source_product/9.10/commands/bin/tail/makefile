default:	tail.c
	 $(CC)  tail.c -o tail
	 findmsg tail.c > tail.msg
	 gencat tail.cat tail.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f tail

# copy /bin/tail to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/tail
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	tail
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp tail ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/tail
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /bin/tail to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/tail
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp tail ../../../Filesets.sl/CMDS-MIN/bin/tail
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/tail
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
