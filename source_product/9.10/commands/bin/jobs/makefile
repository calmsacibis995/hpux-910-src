default:	jobs.sh
	 /bin/cp jobs.sh jobs
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f jobs.sh

# copy /bin/jobs to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/jobs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	jobs
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp jobs ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/jobs
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/jobs to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/jobs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp jobs ../../../Filesets.sl/UX-CORE/bin/jobs
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/jobs
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
