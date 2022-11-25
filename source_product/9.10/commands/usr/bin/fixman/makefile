default:	fixman.sh
	 /bin/cp fixman.sh fixman
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f fixman.sh

# copy /usr/bin/fixman to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/fixman
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	fixman
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp fixman ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/fixman
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
# copy /usr/bin/fixman to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/fixman
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp fixman ../../../../Filesets.sl/TEXT-FMT/usr/bin/fixman
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/fixman
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
	! chmog 555 bin bin ../../../../Filesets.sl/
