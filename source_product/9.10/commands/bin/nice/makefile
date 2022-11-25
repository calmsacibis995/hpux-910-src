default:	nice.c
	 $(CC)  nice.c -o nice
	 findmsg nice.c > nice.msg
	 gencat nice.cat nice.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f nice

# copy /bin/nice to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/nice
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	nice
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp nice ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/nice
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /bin/nice to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/nice
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp nice ../../../Filesets.sl/CMDS-MIN/bin/nice
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/nice
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
