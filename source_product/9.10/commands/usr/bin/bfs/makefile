default:	bfs.c
		$(CC) bfs.c -o bfs
		findmsg bfs.c > bfs.msg
		gencat bfs.cat bfs.msg

clean:
		rm -f *.o *.msg *.cat

clobber:	clean
		rm -f bfs

# copy /usr/bin/bfs to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/bfs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	bfs
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp bfs ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/bfs
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/bfs to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/bfs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp bfs ../../../../Filesets.sl/CMDS-MIN/usr/bin/bfs
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/bfs
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
