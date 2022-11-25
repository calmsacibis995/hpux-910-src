default:	uname.c
	 $(CC)  uname.c -o uname
	 findmsg uname.c > uname.msg
	 gencat uname.cat uname.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f uname

# copy /bin/uname to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/uname
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	uname
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp uname ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/uname
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/uname to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/uname
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp uname ../../../Filesets.sl/UX-CORE/bin/uname
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/uname
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/uname to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/uname
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp uname.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/uname.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/uname.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
