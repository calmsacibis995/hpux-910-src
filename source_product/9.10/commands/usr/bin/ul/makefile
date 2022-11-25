default:	ul.c
	 $(CC) ul.c -o ul -lcurses
	 findmsg ul.c > ul.msg
	 gencat ul.cat ul.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f ul

# copy /usr/bin/ul to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/ul
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	ul
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp ul ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/ul
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
# copy /usr/bin/ul to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/ul
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp ul ../../../../Filesets.sl/TEXT-FMT/usr/bin/ul
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/ul
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
	! chmog 555 bin bin ../../../../Filesets.sl/
