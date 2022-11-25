default:	grep.c
	 $(CC)  grep.c -o grep
	 findmsg grep.c > grep.msg
	 gencat grep.cat grep.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f grep

# copy /bin/grep to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/grep
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	grep
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp grep ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/grep
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/grep to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/grep
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp grep ../../../Filesets.sl/UX-CORE/bin/grep
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/grep
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/grep to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/grep
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp grep.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/grep.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/grep.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/grep to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/grep
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! ln ../../../Filesets.sl/UX-CORE/bin/grep ../../../Filesets.sl/UX-CORE/bin/egrep
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/grep to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/grep
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! ln ../../../Filesets.sl/UX-CORE/bin/grep ../../../Filesets.sl/UX-CORE/bin/fgrep
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
