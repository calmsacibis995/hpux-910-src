default:	dmesg.c
	 $(CC)  dmesg.c -o dmesg
	 findmsg dmesg.c > dmesg.msg
	 gencat dmesg.cat dmesg.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f dmesg

# copy /etc/dmesg to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/dmesg
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	dmesg
	mkdir -p ../../../Filesets.sl/CMDS-MIN/etc
	! cp dmesg ../../../Filesets.sl/CMDS-MIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/etc/dmesg
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /etc/dmesg to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/dmesg
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/etc
	! cp dmesg ../../../Filesets.sl/CMDS-MIN/etc/dmesg
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/etc/dmesg
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
