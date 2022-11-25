default:	lpr.sh
	 /bin/cp lpr.sh lpr
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f lpr.sh

# copy /usr/bin/lpr to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lpr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	lpr
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! cp lpr ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/bin/lpr
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
# copy /usr/bin/lpr to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lpr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! cp lpr ../../../../Filesets.sl/LP-SPOOL/usr/bin/lpr
	! chmog 0555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/bin/lpr
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
