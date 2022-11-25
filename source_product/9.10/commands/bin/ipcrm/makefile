default:	ipcrm.c
	 $(CC)  ipcrm.c -o ipcrm
	 findmsg ipcrm.c > ipcrm.msg
	 gencat ipcrm.cat ipcrm.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f ipcrm

# copy /bin/ipcrm to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /bin/ipcrm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	ipcrm
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/bin
	! cp ipcrm ../../../Filesets.sl/SYS-ADMIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/bin/ipcrm
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /bin/ipcrm to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /bin/ipcrm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/bin
	! cp ipcrm ../../../Filesets.sl/SYS-ADMIN/bin/ipcrm
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/bin/ipcrm
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
