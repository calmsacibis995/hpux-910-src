default:	mesg.c
	 $(CC)  mesg.c -o mesg
	 findmsg mesg.c > mesg.msg
	 gencat mesg.cat mesg.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f mesg

# copy /bin/mesg to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/mesg
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	mesg
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp mesg ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/mesg
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
# copy /bin/mesg to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/mesg
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp mesg ../../../Filesets.sl/CMDS-AUX/bin/mesg
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/mesg
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
