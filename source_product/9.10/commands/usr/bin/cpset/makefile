default:	cpset.c
	 $(CC)  cpset.c -o cpset
	 findmsg cpset.c > cpset.msg
	 gencat cpset.cat cpset.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f cpset

# copy /usr/bin/cpset to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/cpset
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cpset
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp cpset ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin/cpset
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
# copy /usr/bin/cpset to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/cpset
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp cpset ../../../../Filesets.sl/SYS-ADMIN/usr/bin/cpset
	! chmog 0555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin/cpset
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
