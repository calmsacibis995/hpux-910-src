default:	cmp.c
	 $(CC)  cmp.c -o cmp
	 findmsg cmp.c > cmp.msg
	 gencat cmp.cat cmp.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f cmp

# copy /bin/cmp to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/cmp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cmp
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp cmp ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/cmp
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/cmp to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/cmp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp cmp ../../../Filesets.sl/UX-CORE/bin/cmp
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/cmp
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/cmp to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/cmp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp cmp.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/cmp.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/cmp.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
