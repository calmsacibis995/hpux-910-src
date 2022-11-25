default:	mktemp.c
	 $(CC)  mktemp.c -o mktemp
	 findmsg mktemp.c > mktemp.msg
	 gencat mktemp.cat mktemp.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f mktemp

# copy /bin/mktemp to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/mktemp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	mktemp
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp mktemp ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/mktemp
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /bin/mktemp to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/mktemp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp mktemp ../../../Filesets.sl/CMDS-MIN/bin/mktemp
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/mktemp
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
