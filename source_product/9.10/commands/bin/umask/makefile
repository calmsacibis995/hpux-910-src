default:	umask.sh
	 /bin/cp umask.sh umask
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f umask.sh

# copy /bin/umask to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/umask
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	umask
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp umask ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/umask
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/umask to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/umask
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp umask ../../../Filesets.sl/UX-CORE/bin/umask
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/umask
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
