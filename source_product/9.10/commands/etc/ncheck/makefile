default:	ncheck.c
	 $(CC)  ncheck.c -o ncheck
	 findmsg ncheck.c > ncheck.msg
	 gencat ncheck.cat ncheck.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f ncheck

# copy /etc/ncheck to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/ncheck
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	ncheck
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp ncheck ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/ncheck
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/ncheck to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/ncheck
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp ncheck ../../../Filesets.sl/SYS-ADMIN/etc/ncheck
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/ncheck
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
