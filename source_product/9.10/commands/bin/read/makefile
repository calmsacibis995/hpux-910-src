default:	read.sh
	 /bin/cp read.sh read
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f read.sh

# copy /bin/read to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/read
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	read
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp read ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/read
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/read to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/read
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp read ../../../Filesets.sl/UX-CORE/bin/read
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/read
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
