default:	fg.sh
	 /bin/cp fg.sh fg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f fg.sh

# copy /bin/fg to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/fg
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	fg
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp fg ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/fg
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/fg to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/fg
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp fg ../../../Filesets.sl/UX-CORE/bin/fg
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/fg
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
