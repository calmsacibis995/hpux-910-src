default:
	cp backup.sh backup

# copy /etc/backup to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/backup
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc/newconfig
	! cp backup ../../../Filesets.sl/SYS-ADMIN/etc/newconfig/backup
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/newconfig/backup
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/newconfig
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
