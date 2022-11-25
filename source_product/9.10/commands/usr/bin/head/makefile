default:	head.c
		$(CC) head.c -o head
		findmsg head.c > head.msg
		gencat head.cat head.msg

clean:
		rm -f *.o *.msg *.cat

clobber:	clean
		rm -f head

# copy /usr/bin/head to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/head
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	head
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp head ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/head
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/head to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/head
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp head ../../../../Filesets.sl/CMDS-MIN/usr/bin/head
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/head
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
