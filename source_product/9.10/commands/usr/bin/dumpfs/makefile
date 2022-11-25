default:	dumpfs.c
	 $(CC)  dumpfs.c -o dumpfs
	 findmsg dumpfs.c > dumpfs.msg
	 gencat dumpfs.cat dumpfs.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f dumpfs

# copy /usr/bin/dumpfs to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/dumpfs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	dumpfs
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp dumpfs ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin/dumpfs
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
# copy /usr/bin/dumpfs to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/dumpfs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp dumpfs ../../../../Filesets.sl/SYS-ADMIN/usr/bin/dumpfs
	! chmog 0555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin/dumpfs
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
