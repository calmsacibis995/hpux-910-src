default:	umount.c
	 $(CC)  umount.c -o umount
	 findmsg umount.c > umount.msg
	 gencat umount.cat umount.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f umount

# copy /etc/umount to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/umount
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	umount
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp umount ../../../Filesets.sl/UX-CORE/etc
	! chmog 0544 bin bin ../../../Filesets.sl/UX-CORE/etc/umount
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/umount to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/umount
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp umount ../../../Filesets.sl/UX-CORE/etc/umount
	! chmog 0544 bin bin ../../../Filesets.sl/UX-CORE/etc/umount
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
