default:	diss.c
	$(CC) -I/usr/contrib/include diss.c -o diss

# copy /usr/contrib/bin/diss to the . relative fileset USRCONTRB
# change mode, owner, and group on /usr/contrib/bin/diss
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../../Filesets.sl/USRCONTRB/usr/contrib/bin
	! cp diss ../../../../../Filesets.sl/USRCONTRB/usr/contrib/bin/diss
	! chmog 0555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/contrib/bin/diss
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/contrib/bin
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/contrib/
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/
	! chmog 555 bin bin ../../../../../Filesets.sl/
# copy /usr/contrib/bin/diss to the . relative fileset USRCONTRB
# change mode, owner, and group on /usr/contrib/bin/diss
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../../Filesets.sl/USRCONTRB/usr/contrib/man/man1
	! cp diss.1 ../../../../../Filesets.sl/USRCONTRB/usr/contrib/man/man1/diss.1
	! chmog 0555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/contrib/man/man1/diss.1
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/contrib/man/man1
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/contrib/man/
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/contrib/
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/usr/
	! chmog 555 bin bin ../../../../../Filesets.sl/USRCONTRB/
	! chmog 555 bin bin ../../../../../Filesets.sl/
