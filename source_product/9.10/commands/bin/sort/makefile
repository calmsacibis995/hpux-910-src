default:	sort.c
	 $(CC) -I/usr/src/lib/libc/gen sort.c -o sort
	 findmsg sort.c > sort.msg
	 gencat sort.cat sort.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f sort

# copy /bin/sort to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sort
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	sort
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp sort ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/sort
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/sort to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sort
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp sort ../../../Filesets.sl/UX-CORE/bin/sort
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/sort
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/sort to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sort
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp sort.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/sort.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/sort.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
