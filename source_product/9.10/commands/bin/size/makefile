default:	size.c
	 $(CC)  size.c -o size
	 findmsg size.c > size.msg
	 gencat size.cat size.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f size

# copy /bin/size to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/size
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	size
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp size ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/size
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
# copy /bin/size to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/size
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp size ../../../Filesets.sl/CMDS-AUX/bin/size
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/size
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
