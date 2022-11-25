default:	grpck.c
	 $(CC)  grpck.c -o grpck
	 findmsg grpck.c > grpck.msg
	 gencat grpck.cat grpck.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f grpck

# copy /etc/grpck to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/grpck
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	grpck
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp grpck ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/grpck
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/grpck to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/grpck
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp grpck ../../../Filesets.sl/SYS-ADMIN/etc/grpck
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/grpck
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
