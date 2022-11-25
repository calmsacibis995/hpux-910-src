default:	hp.c
	 $(CC)  hp.c -o hp
	 findmsg hp.c > hp.msg
	 gencat hp.cat hp.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f hp

# copy /usr/bin/hp to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/hp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	hp
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp hp ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/hp
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/hp to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/hp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp hp ../../../../Filesets.sl/CMDS-MIN/usr/bin/hp
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/hp
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
