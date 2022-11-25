default:	diskinfo.c
	 $(CC)  diskinfo.c -o diskinfo
	 findmsg diskinfo.c > diskinfo.msg
	 gencat diskinfo.cat diskinfo.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f diskinfo

# copy /etc/diskinfo to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/diskinfo
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	diskinfo
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp diskinfo ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/diskinfo
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/diskinfo to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/diskinfo
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp diskinfo ../../../Filesets.sl/UX-CORE/etc/diskinfo
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/diskinfo
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
