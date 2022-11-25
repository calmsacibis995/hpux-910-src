default:	mkfs.c
	 $(CC)  mkfs.c -o mkfs
	 $(CC)  -Wl,-a,archive mkfs.c -o mkfs.arch
	 findmsg mkfs.c > mkfs.msg
	 gencat mkfs.cat mkfs.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f mkfs

# copy /etc/mkfs to the . relative fileset INSTALL
# change mode, owner, and group on /etc/mkfs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	mkfs
# copy /etc/mkfs to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/mkfs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp mkfs ../../../Filesets.sl/UX-CORE/etc/mkfs
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/mkfs
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/mkfs to the . relative fileset INSTALL
# change mode, owner, and group on /etc/mkfs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/INSTALL/etc
	! cp mkfs.arch ../../../Filesets.sl/INSTALL/etc/mkfs
	! chmog 0555 bin bin ../../../Filesets.sl/INSTALL/etc/mkfs
	! chmog 555 bin bin ../../../Filesets.sl/INSTALL/etc
	! chmog 555 bin bin ../../../Filesets.sl/INSTALL/
	! chmog 555 bin bin ../../../Filesets.sl/
