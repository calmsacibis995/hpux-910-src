default:	lsacl.c
	 $(CC)  lsacl.c -o lsacl
	 findmsg lsacl.c > lsacl.msg
	 gencat lsacl.cat lsacl.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f lsacl

# copy /bin/lsacl to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/lsacl
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	lsacl
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp lsacl ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/lsacl
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /bin/lsacl to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/lsacl
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp lsacl ../../../Filesets.sl/CMDS-MIN/bin/lsacl
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/lsacl
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/lsacl to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/lsacl
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp lsacl.cat ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/lsacl.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/lsacl.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
