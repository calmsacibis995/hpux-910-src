default:	getopts.sh
	 /bin/cp getopts.sh getopts
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f getopts.sh

# copy /bin/getopts to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/getopts
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	getopts
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp getopts ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/getopts
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/getopts to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/getopts
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp getopts ../../../Filesets.sl/UX-CORE/bin/getopts
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/getopts
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
