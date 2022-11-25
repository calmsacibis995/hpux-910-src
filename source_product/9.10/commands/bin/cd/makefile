default:	cd.sh
	 /bin/cp cd.sh cd
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f cd.sh

# copy /bin/cd to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/cd
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cd
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp cd ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/cd
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/cd to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/cd
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp cd ../../../Filesets.sl/UX-CORE/bin/cd
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/cd
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
