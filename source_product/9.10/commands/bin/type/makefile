default:	type.sh
	 /bin/cp type.sh type
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f type.sh

# copy /bin/type to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/type
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	type
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp type ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/type
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/type to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/type
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp type ../../../Filesets.sl/UX-CORE/bin/type
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/type
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
