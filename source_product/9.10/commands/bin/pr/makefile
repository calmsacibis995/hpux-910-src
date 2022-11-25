default:	pr.c
	 $(CC)  pr.c -o pr
	 findmsg pr.c > pr.msg
	 gencat pr.cat pr.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f pr

# copy /bin/pr to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/pr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	pr
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp pr ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/pr
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /bin/pr to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/pr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp pr ../../../Filesets.sl/CMDS-MIN/bin/pr
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/pr
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/pr to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/pr
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp pr.cat ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/pr.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/pr.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
