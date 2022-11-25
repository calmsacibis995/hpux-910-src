default:	untic.c
	 $(CC) untic.c -o untic -lcurses
	 findmsg untic.c > untic.msg
	 gencat untic.cat untic.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f untic

# copy /usr/bin/untic to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/untic
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	untic
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp untic ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin/untic
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
# copy /usr/bin/untic to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/untic
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp untic ../../../../Filesets.sl/SYS-ADMIN/usr/bin/untic
	! chmog 0555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin/untic
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
