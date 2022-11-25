default:	bg.sh
	 /bin/cp bg.sh bg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f bg.sh

# copy /bin/bg to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/bg
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	bg
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp bg ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/bg
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/bg to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/bg
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp bg ../../../Filesets.sl/UX-CORE/bin/bg
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/bg
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
