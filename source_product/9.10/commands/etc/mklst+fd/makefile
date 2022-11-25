default:
	cp mklst+fd.sh mklost+found

# copy /etc/mklst+fd to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/mklst+fd
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp mklost+found ../../../Filesets.sl/SYS-ADMIN/etc/mklost+found
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/mklost+found
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
