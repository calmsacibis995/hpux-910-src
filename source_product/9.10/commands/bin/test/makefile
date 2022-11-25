default:	test.sh
	 /bin/cp test.sh test
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f test.sh

# copy /bin/test to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/test
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	test
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp test ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/test
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/test to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/test
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp test ../../../Filesets.sl/UX-CORE/bin/test
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/test
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
