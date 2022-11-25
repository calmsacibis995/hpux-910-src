default:	fsdb.c
	 $(CC)  fsdb.c -o fsdb
	 findmsg fsdb.c > fsdb.msg
	 gencat fsdb.cat fsdb.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f fsdb

# copy /etc/fsdb to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/fsdb
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	fsdb
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp fsdb ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/fsdb
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/fsdb to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/fsdb
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp fsdb ../../../Filesets.sl/SYS-ADMIN/etc/fsdb
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/fsdb
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
