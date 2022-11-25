default:	time.c
	 $(CC)  time.c -o time
	 findmsg time.c > time.msg
	 gencat time.cat time.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f time

# copy /bin/time to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/time
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	time
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp time ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/time
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
# copy /bin/time to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/time
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp time ../../../Filesets.sl/CMDS-AUX/bin/time
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/time
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
