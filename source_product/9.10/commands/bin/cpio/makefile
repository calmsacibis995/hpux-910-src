default:	cpio.c
	 $(CC)  cpio.c -o cpio
	 findmsg cpio.c > cpio.msg
	 gencat cpio.cat cpio.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f cpio

# copy /bin/cpio to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/cpio
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cpio
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp cpio ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/cpio
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/cpio to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/cpio
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp cpio ../../../Filesets.sl/UX-CORE/bin/cpio
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/cpio
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/cpio to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/cpio
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp cpio.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/cpio.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/cpio.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
