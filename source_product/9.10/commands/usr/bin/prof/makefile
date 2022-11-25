default:	prof.c
		$(CC) prof.c -o prof
		findmsg prof.c > prof.msg
		gencat prof.cat prof.msg

clean:
		rm -f *.o *.msg *.cat

clobber:	clean
		rm -f prof

# copy /usr/bin/prof to the . relative fileset PROG-MIN
# change mode, owner, and group on /usr/bin/prof
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	prof
	mkdir -p ../../../../Filesets.sl/PROG-MIN/usr/bin
	! cp prof ../../../../Filesets.sl/PROG-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-MIN/usr/bin/prof
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-MIN/
# copy /usr/bin/prof to the . relative fileset PROG-MIN
# change mode, owner, and group on /usr/bin/prof
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-MIN/usr/bin
	! cp prof ../../../../Filesets.sl/PROG-MIN/usr/bin/prof
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-MIN/usr/bin/prof
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
