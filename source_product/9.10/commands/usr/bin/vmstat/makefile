default:	vmstat.c
	 $(CC)  vmstat.c -o vmstat
	 findmsg vmstat.c > vmstat.msg
	 gencat vmstat.cat vmstat.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f vmstat

# copy /usr/bin/vmstat to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/vmstat
# to 02555, bin, and sys
# change mode, owner, and group on all the parent
# directories.

install:	vmstat
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp vmstat ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 02555 bin sys ../../../../Filesets.sl/SYS-ADMIN/usr/bin/vmstat
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
# copy /usr/bin/vmstat to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/vmstat
# to 02555, bin, and sys
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp vmstat ../../../../Filesets.sl/SYS-ADMIN/usr/bin/vmstat
	! chmog 02555 bin sys ../../../../Filesets.sl/SYS-ADMIN/usr/bin/vmstat
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
