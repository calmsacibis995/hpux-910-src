default:	diffmk.sh
	 /bin/cp diffmk.sh diffmk
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f diffmk.sh

# copy /usr/bin/diffmk to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/diffmk
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	diffmk
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp diffmk ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/diffmk
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
# copy /usr/bin/diffmk to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/diffmk
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp diffmk ../../../../Filesets.sl/TEXT-FMT/usr/bin/diffmk
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/diffmk
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
	! chmog 555 bin bin ../../../../Filesets.sl/
