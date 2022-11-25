default:	vipw.c
		$(CC) vipw.c -o vipw
		findmsg vipw.c > vipw.msg
		gencat vipw.cat vipw.msg

clean:
		rm -f *.o *.msg *.cat

clobber:	clean
		rm -f vipw

# copy /usr/bin/vipw to the . relative fileset EDITORS
# change mode, owner, and group on /usr/bin/vipw
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	vipw
	mkdir -p ../../../../Filesets.sl/EDITORS/usr/bin
	! cp vipw ../../../../Filesets.sl/EDITORS/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/EDITORS/usr/bin/vipw
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/
