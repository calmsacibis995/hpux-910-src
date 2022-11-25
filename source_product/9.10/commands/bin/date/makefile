default:	date.c
	 $(CC)  date.c -o date
	 findmsg date.c > date.msg
	 gencat date.cat date.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f date

# copy /bin/date to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/date
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	date
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp date ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/date
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/date to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/date
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp date ../../../Filesets.sl/UX-CORE/bin/date
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/date
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/date to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/date
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp date.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/date.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/date.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
