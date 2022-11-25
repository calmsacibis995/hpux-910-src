default:	hyphen.c
		$(CC) hyphen.c -o hyphen
		findmsg hyphen.c > hyphen.msg
		gencat hyphen.cat hyphen.msg

clean:
		rm -f *.o *.msg *.cat

clobber:	clean
		rm -f hyphen

# copy /usr/bin/hyphen to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/hyphen
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	hyphen
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp hyphen ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/hyphen
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
# copy /usr/bin/hyphen to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/hyphen
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp hyphen ../../../../Filesets.sl/TEXT-FMT/usr/bin/hyphen
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/hyphen
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
	! chmog 555 bin bin ../../../../Filesets.sl/
