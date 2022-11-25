default:	last.c
	 $(CC)  last.c -o last
	 findmsg last.c > last.msg
	 gencat last.cat last.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f last

# copy /etc/last to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/last
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	last
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp last ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/last
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/last to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/last
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp last ../../../Filesets.sl/SYS-ADMIN/etc/last
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/last
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/last to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/last
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! ln ../../../Filesets.sl/SYS-ADMIN/etc/last ../../../Filesets.sl/SYS-ADMIN/etc/lastb
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
