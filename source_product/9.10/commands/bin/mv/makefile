default:	mv.c
	 $(CC)  mv.c -o mv
	 findmsg mv.c > mv.msg
	 gencat mv.cat mv.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f mv

# copy /bin/mv to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/mv
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	mv
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp mv ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/mv
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/mv to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/mv
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp mv ../../../Filesets.sl/UX-CORE/bin/mv
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/mv
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/mv to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/mv
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp mv.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/mv.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/mv.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
