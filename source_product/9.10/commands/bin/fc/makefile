default:	fc.sh
	 /bin/cp fc.sh fc
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f fc.sh

# copy /bin/fc to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/fc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	fc
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp fc ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/fc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/fc to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/fc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp fc ../../../Filesets.sl/UX-CORE/bin/fc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/fc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
