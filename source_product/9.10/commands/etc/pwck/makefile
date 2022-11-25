default:	pwck.c
	 $(CC)  pwck.c -o pwck
	 findmsg pwck.c > pwck.msg
	 gencat pwck.cat pwck.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f pwck

# copy /etc/pwck to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/pwck
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	pwck
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp pwck ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/pwck
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/pwck to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/pwck
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp pwck ../../../Filesets.sl/SYS-ADMIN/etc/pwck
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/pwck
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
