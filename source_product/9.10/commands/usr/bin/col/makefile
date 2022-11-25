default:	col.c
	 $(CC)  col.c -o col
	 findmsg col.c > col.msg
	 gencat col.cat col.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f col

# copy /usr/bin/col to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/col
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	col
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp col ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/col
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
# copy /usr/bin/col to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/col
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp col ../../../../Filesets.sl/TEXT-FMT/usr/bin/col
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/col
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/col to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/col
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/lib/nls/C
	! cp col.cat ../../../../Filesets.sl/TEXT-FMT/usr/lib/nls/C/col.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/lib/nls/C/col.cat
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
	! chmog 555 bin bin ../../../../Filesets.sl/
