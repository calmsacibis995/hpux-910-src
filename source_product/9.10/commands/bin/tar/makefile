default:	tar.c
	 $(CC) -Wl,-a,archive  tar.c -o tar
	 findmsg tar.c > tar.msg
	 gencat tar.cat tar.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f tar

# copy /bin/tar to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/tar
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	tar
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp tar ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/tar
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/tar to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/tar
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp tar ../../../Filesets.sl/UX-CORE/bin/tar
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/tar
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/tar to the . relative fileset INSTALL
# change mode, owner, and group on /bin/tar
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/INSTALL/etc
	! cp tar ../../../Filesets.sl/INSTALL/etc/tar
	! chmog 0555 bin bin ../../../Filesets.sl/INSTALL/etc/tar
	! chmog 555 bin bin ../../../Filesets.sl/INSTALL/etc
	! chmog 555 bin bin ../../../Filesets.sl/INSTALL/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/tar to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/tar
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp tar.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/tar.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/tar.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
