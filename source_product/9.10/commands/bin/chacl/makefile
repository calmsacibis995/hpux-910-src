default:	chacl.c
	 $(CC)  chacl.c -o chacl
	 findmsg chacl.c > chacl.msg
	 gencat chacl.cat chacl.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f chacl

# copy /bin/chacl to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/chacl
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	chacl
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp chacl ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/chacl
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /bin/chacl to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/chacl
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp chacl ../../../Filesets.sl/CMDS-MIN/bin/chacl
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/chacl
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/chacl to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/chacl
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp chacl.cat ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/chacl.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/chacl.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
