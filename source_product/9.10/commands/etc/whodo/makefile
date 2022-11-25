default:	whodo.sh
	 /bin/cp whodo.sh whodo
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f whodo.sh

# copy /etc/whodo to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/whodo
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	whodo
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp whodo ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/whodo
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/whodo to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/whodo
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp whodo ../../../Filesets.sl/SYS-ADMIN/etc/whodo
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/whodo
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
