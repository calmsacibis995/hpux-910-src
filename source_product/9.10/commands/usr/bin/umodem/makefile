default:	umodem.c
	 $(CC)  umodem.c -o umodem
	 findmsg umodem.c > umodem.msg
	 gencat umodem.cat umodem.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f umodem

# copy /usr/bin/umodem to the . relative fileset SYSCOM
# change mode, owner, and group on /usr/bin/umodem
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	umodem
	mkdir -p ../../../../Filesets.sl/SYSCOM/usr/bin
	! cp umodem ../../../../Filesets.sl/SYSCOM/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin/umodem
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
# copy /usr/bin/umodem to the . relative fileset SYSCOM
# change mode, owner, and group on /usr/bin/umodem
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYSCOM/usr/bin
	! cp umodem ../../../../Filesets.sl/SYSCOM/usr/bin/umodem
	! chmog 0555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin/umodem
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
	! chmog 555 bin bin ../../../../Filesets.sl/
