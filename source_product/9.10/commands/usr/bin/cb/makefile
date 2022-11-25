default:	cb.c
		$(CC) cb.c -o cb
		findmsg cb.c > cb.msg
		gencat cb.cat cb.msg

clean:
		rm -f *.o *.msg *.cat

clobber:	clean
		rm -f cb

# copy /usr/bin/cb to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/cb
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cb
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/bin
	! cp cb ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin/cb
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
# copy /usr/bin/cb to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/cb
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/bin
	! cp cb ../../../../Filesets.sl/C-TOOLS/usr/bin/cb
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin/cb
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
