default:	checknr.c
	 $(CC)  checknr.c -o checknr
	 findmsg checknr.c > checknr.msg
	 gencat checknr.cat checknr.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f checknr

# copy /usr/bin/checknr to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/checknr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	checknr
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp checknr ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/checknr
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
# copy /usr/bin/checknr to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/checknr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp checknr ../../../../Filesets.sl/TEXT-FMT/usr/bin/checknr
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/checknr
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
	! chmog 555 bin bin ../../../../Filesets.sl/
