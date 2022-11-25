default:	newfs.c
	 $(CC)  newfs.c -o newfs
	 findmsg newfs.c > newfs.msg
	 gencat newfs.cat newfs.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f newfs

# copy /etc/newfs to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/newfs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	newfs
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp newfs ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/newfs
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/newfs to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/newfs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp newfs ../../../Filesets.sl/UX-CORE/etc/newfs
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/newfs
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
