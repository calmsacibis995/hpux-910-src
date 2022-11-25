default:	prealloc.c
	 $(CC)  prealloc.c -o prealloc
	 findmsg prealloc.c > prealloc.msg
	 gencat prealloc.cat prealloc.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f prealloc

# copy /usr/bin/prealloc to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/prealloc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	prealloc
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp prealloc ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin/prealloc
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
# copy /usr/bin/prealloc to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/prealloc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp prealloc ../../../../Filesets.sl/SYS-ADMIN/usr/bin/prealloc
	! chmog 0555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin/prealloc
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
