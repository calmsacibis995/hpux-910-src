default:	tee.c
	 $(CC)  tee.c -o tee
	 findmsg tee.c > tee.msg
	 gencat tee.cat tee.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f tee

# copy /bin/tee to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/tee
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	tee
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp tee ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/tee
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /bin/tee to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/tee
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp tee ../../../Filesets.sl/CMDS-MIN/bin/tee
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/tee
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/tee to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/tee
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp tee.cat ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/tee.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/tee.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
