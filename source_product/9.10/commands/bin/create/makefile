default:	create.c
	 $(CC)  create.c -o create
	 findmsg create.c > create.msg
	 gencat create.cat create.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f create
# copy /bin/create to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/create
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
# copy /bin/create to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/create
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp create ../../../Filesets.sl/UX-CORE/bin/mkdir
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/mkdir
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/create to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/create
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp create.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/mkdir.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/mkdir.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! ln ../../../Filesets.sl/UX-CORE/bin/mkdir ../../../Filesets.sl/UX-CORE/bin/mkfifo
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
