default:	vacation.c
	 $(CC) vacation.c -o vacation -lndbm
	 findmsg vacation.c > vacation.msg
	 gencat vacation.cat vacation.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f vacation

# copy /usr/bin/vacation to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/vacation
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	vacation
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/bin
	! cp vacation ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin/vacation
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
# copy /usr/bin/vacation to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/vacation
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/bin
	! cp vacation ../../../../Filesets.sl/MAILERS/usr/bin/vacation
	! chmog 0555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin/vacation
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/vacation to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/vacation
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/lib/nls/C
	! cp vacation.cat ../../../../Filesets.sl/MAILERS/usr/lib/nls/C/vacation.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/C/vacation.cat
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
