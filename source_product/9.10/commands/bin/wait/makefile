default:	wait.sh
	 /bin/cp wait.sh wait
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f wait.sh

# copy /bin/wait to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/wait
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	wait
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp wait ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/wait
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/wait to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/wait
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp wait ../../../Filesets.sl/UX-CORE/bin/wait
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/wait
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
