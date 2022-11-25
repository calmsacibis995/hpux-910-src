default:	ipcs.c
	 $(CC)  ipcs.c -o ipcs
	 findmsg ipcs.c > ipcs.msg
	 gencat ipcs.cat ipcs.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f ipcs

# copy /bin/ipcs to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/ipcs
# to 02555, bin, and sys
# change mode, owner, and group on all the parent
# directories.

install:	ipcs
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp ipcs ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 02555 bin sys ../../../Filesets.sl/CMDS-AUX/bin/ipcs
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
# copy /bin/ipcs to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/ipcs
# to 02555, bin, and sys
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp ipcs ../../../Filesets.sl/CMDS-AUX/bin/ipcs
	! chmog 02555 bin sys ../../../Filesets.sl/CMDS-AUX/bin/ipcs
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
