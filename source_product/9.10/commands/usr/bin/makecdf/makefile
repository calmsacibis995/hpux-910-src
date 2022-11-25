default:	makecdf.c
	 $(CC) -Wl,-a,archive  makecdf.c -o makecdf
	 findmsg makecdf.c > makecdf.msg
	 gencat makecdf.cat makecdf.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f makecdf

# copy /usr/bin/makecdf to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/makecdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	makecdf
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/bin
	! cp makecdf ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin/makecdf
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
# copy /usr/bin/makecdf to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/makecdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/bin
	! cp makecdf ../../../../Filesets.sl/UX-CORE/usr/bin/makecdf
	! chmog 0555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin/makecdf
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
