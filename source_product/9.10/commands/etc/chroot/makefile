default:	chroot.c
	 $(CC)  chroot.c -o chroot
	 findmsg chroot.c > chroot.msg
	 gencat chroot.cat chroot.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f chroot

# copy /etc/chroot to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/chroot
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	chroot
	mkdir -p ../../../Filesets.sl/CMDS-MIN/etc
	! cp chroot ../../../Filesets.sl/CMDS-MIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/etc/chroot
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /etc/chroot to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/chroot
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/etc
	! cp chroot ../../../Filesets.sl/CMDS-MIN/etc/chroot
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/etc/chroot
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
