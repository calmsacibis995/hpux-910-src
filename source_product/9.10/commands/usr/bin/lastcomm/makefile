default:	lastcomm.c
	 $(CC)  lastcomm.c -o lastcomm
	 findmsg lastcomm.c > lastcomm.msg
	 gencat lastcomm.cat lastcomm.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f lastcomm

# copy /usr/bin/lastcomm to the . relative fileset ACCOUNTNG
# change mode, owner, and group on /usr/bin/lastcomm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	lastcomm
	mkdir -p ../../../../Filesets.sl/ACCOUNTNG/usr/bin
	! cp lastcomm ../../../../Filesets.sl/ACCOUNTNG/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/ACCOUNTNG/usr/bin/lastcomm
	! chmog 555 bin bin ../../../../Filesets.sl/ACCOUNTNG/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/ACCOUNTNG/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ACCOUNTNG/
# copy /usr/bin/lastcomm to the . relative fileset ACCOUNTNG
# change mode, owner, and group on /usr/bin/lastcomm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ACCOUNTNG/usr/bin
	! cp lastcomm ../../../../Filesets.sl/ACCOUNTNG/usr/bin/lastcomm
	! chmog 0555 bin bin ../../../../Filesets.sl/ACCOUNTNG/usr/bin/lastcomm
	! chmog 555 bin bin ../../../../Filesets.sl/ACCOUNTNG/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/ACCOUNTNG/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ACCOUNTNG/
	! chmog 555 bin bin ../../../../Filesets.sl/
