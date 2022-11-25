default:	id.c
	 $(CC)  id.c -o id
	 findmsg id.c > id.msg
	 gencat id.cat id.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f id

# copy /usr/bin/id to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/id
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	id
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp id ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/id
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/id to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/id
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp id ../../../../Filesets.sl/CMDS-MIN/usr/bin/id
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/id
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/id to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/id
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp id.cat ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/id.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/id.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
