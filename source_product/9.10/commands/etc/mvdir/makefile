default:	mvdir.sh
	 /bin/cp mvdir.sh mvdir
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f mvdir.sh

# copy /etc/mvdir to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/mvdir
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	mvdir
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp mvdir ../../../Filesets.sl/UX-CORE/etc
	! chmog 0544 bin bin ../../../Filesets.sl/UX-CORE/etc/mvdir
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/mvdir to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/mvdir
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp mvdir ../../../Filesets.sl/UX-CORE/etc/mvdir
	! chmog 0544 bin bin ../../../Filesets.sl/UX-CORE/etc/mvdir
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
