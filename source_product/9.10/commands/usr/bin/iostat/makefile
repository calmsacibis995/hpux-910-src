default:	iostat.c
	 $(CC)  iostat.c -o iostat
	 findmsg iostat.c > iostat.msg
	 gencat iostat.cat iostat.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f iostat

# copy /usr/bin/iostat to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/iostat
# to 02555, bin, and sys
# change mode, owner, and group on all the parent
# directories.

install:	iostat
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp iostat ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 02555 bin sys ../../../../Filesets.sl/SYS-ADMIN/usr/bin/iostat
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
# copy /usr/bin/iostat to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/iostat
# to 02555, bin, and sys
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp iostat ../../../../Filesets.sl/SYS-ADMIN/usr/bin/iostat
	! chmog 02555 bin sys ../../../../Filesets.sl/SYS-ADMIN/usr/bin/iostat
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
