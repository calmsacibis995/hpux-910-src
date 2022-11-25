default:	prmail.c
	 $(CC)  prmail.c -o prmail
	 findmsg prmail.c > prmail.msg
	 gencat prmail.cat prmail.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f prmail

# copy /usr/bin/prmail to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/prmail
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	prmail
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/bin
	! cp prmail ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin/prmail
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
# copy /usr/bin/prmail to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/prmail
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/bin
	! cp prmail ../../../../Filesets.sl/MAILERS/usr/bin/prmail
	! chmog 0555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin/prmail
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
