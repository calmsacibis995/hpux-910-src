default:	lsdev.c
	 $(CC)  lsdev.c -o lsdev
	 findmsg lsdev.c > lsdev.msg
	 gencat lsdev.cat lsdev.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f lsdev

# copy /etc/lsdev to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/lsdev
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	lsdev
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp lsdev ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/lsdev
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/lsdev to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/lsdev
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp lsdev ../../../Filesets.sl/SYS-ADMIN/etc/lsdev
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/lsdev
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
