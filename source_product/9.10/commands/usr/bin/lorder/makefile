default:	lorder.sh
	 /bin/cp lorder.sh lorder
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f lorder.sh

# copy /usr/bin/lorder to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/lorder
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	lorder
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/bin
	! cp lorder ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin/lorder
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
# copy /usr/bin/lorder to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/lorder
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/bin
	! cp lorder ../../../../Filesets.sl/PROG-AUX/usr/bin/lorder
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin/lorder
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
