default:	das.c
	 $(CC)  das.c -o das
	 findmsg das.c > das.msg
	 gencat das.cat das.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f das

# copy /usr/contrib/bin/das to the . relative fileset USRCONTRB
# change mode, owner, and group on /usr/contrib/bin/das
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	das
	mkdir -p ../../../../../Filesets.sl/USRCONTRB/usr/contrib/bin
	! cp das ../../../../../Filesets.sl/USRCONTRB/usr/contrib/bin
	! chmog 0555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/contrib/bin/das
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/contrib/bin
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/contrib/
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/
# copy /usr/contrib/bin/das to the . relative fileset USRCONTRB
# change mode, owner, and group on /usr/contrib/bin/das
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../../Filesets.sl/USRCONTRB/usr/contrib/bin
	! cp das ../../../../../Filesets.sl/USRCONTRB/usr/contrib/bin/das
	! chmog 0555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/contrib/bin/das
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/contrib/bin
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/contrib/
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/
	! chmog 555 bin bin ../../../../../Filesets.sl/
