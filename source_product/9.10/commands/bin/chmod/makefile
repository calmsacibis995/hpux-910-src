default:	chmod.c
	 $(CC) chmod.c -o chmod -lBUILD
	 findmsg chmod.c > chmod.msg
	 gencat chmod.cat chmod.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f chmod

# copy /bin/chmod to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/chmod
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	chmod
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp chmod ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/chmod
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/chmod to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/chmod
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp chmod ../../../Filesets.sl/UX-CORE/bin/chmod
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/chmod
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/chmod to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/chmod
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp chmod.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/chmod.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/chmod.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
