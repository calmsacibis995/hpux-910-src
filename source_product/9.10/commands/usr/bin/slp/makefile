default:	slp.c
	 $(CC)  slp.c -o slp
	 findmsg slp.c > slp.msg
	 gencat slp.cat slp.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f slp

# copy /usr/bin/slp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/slp
# to 04555, lp, and bin
# change mode, owner, and group on all the parent
# directories.

install:	slp
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! cp slp ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! chmog 04555 lp bin ../../../../Filesets.sl/LP-SPOOL/usr/bin/slp
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
# copy /usr/bin/slp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/slp
# to 04555, lp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! cp slp ../../../../Filesets.sl/LP-SPOOL/usr/bin/slp
	! chmog 04555 lp bin ../../../../Filesets.sl/LP-SPOOL/usr/bin/slp
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
