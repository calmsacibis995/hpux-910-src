default:	unalias.sh
	 /bin/cp unalias.sh unalias
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f unalias.sh

# copy /bin/unalias to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/unalias
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	unalias
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp unalias ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/unalias
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/unalias to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/unalias
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp unalias ../../../Filesets.sl/UX-CORE/bin/unalias
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/unalias
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
