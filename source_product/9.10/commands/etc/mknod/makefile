default:	mknod.c
	 $(CC)  mknod.c -o mknod
	 findmsg mknod.c > mknod.msg
	 gencat mknod.cat mknod.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f mknod

# copy /etc/mknod to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/mknod
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	mknod
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp mknod ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/mknod
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/mknod to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/mknod
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp mknod ../../../Filesets.sl/UX-CORE/etc/mknod
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/mknod
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
