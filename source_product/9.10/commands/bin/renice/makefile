default:	renice.c
	 $(CC)  renice.c -o renice
	 findmsg renice.c > renice.msg
	 gencat renice.cat renice.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f renice

# copy /bin/renice to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/renice
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	renice
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp renice ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/renice
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /bin/renice to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/renice
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp renice ../../../Filesets.sl/CMDS-MIN/bin/renice
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/renice
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
