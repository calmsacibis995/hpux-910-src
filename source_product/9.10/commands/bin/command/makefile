default:	command.sh
	 /bin/cp command.sh command
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f command.sh

# copy /bin/command to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/command
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	command
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp command ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/command
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/command to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/command
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp command ../../../Filesets.sl/UX-CORE/bin/command
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/command
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
