default:	convertfs.c
	 $(CC)  convertfs.c -o convertfs
	 findmsg convertfs.c > convertfs.msg
	 gencat convertfs.cat convertfs.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f convertfs

# copy /etc/convertfs to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/convertfs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	convertfs
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp convertfs ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/convertfs
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/convertfs to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/convertfs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp convertfs ../../../Filesets.sl/UX-CORE/etc/convertfs
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/convertfs
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
