default:	cp.c
	 $(CC)  cp.c -o cp
	 findmsg cp.c > cp.msg
	 gencat cp.cat cp.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f cp

# copy /bin/cp to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/cp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cp
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp cp ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/cp
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/cp to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/cp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp cp ../../../Filesets.sl/UX-CORE/bin/cp
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/cp
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/cp to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/cp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp cp.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/cp.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/cp.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
