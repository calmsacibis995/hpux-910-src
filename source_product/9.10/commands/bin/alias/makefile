default:	alias.sh
	 /bin/cp alias.sh alias
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f alias.sh

# copy /bin/alias to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/alias
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	alias
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp alias ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/alias
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/alias to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/alias
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp alias ../../../Filesets.sl/UX-CORE/bin/alias
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/alias
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
