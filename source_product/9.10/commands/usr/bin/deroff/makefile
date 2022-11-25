default:	deroff.c
	 $(CC)  deroff.c -o deroff
	 findmsg deroff.c > deroff.msg
	 gencat deroff.cat deroff.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f deroff

# copy /usr/bin/deroff to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/deroff
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	deroff
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp deroff ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/deroff
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
# copy /usr/bin/deroff to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/deroff
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp deroff ../../../../Filesets.sl/TEXT-FMT/usr/bin/deroff
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/deroff
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/deroff to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/deroff
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/lib/nls/C
	! cp deroff.cat ../../../../Filesets.sl/TEXT-FMT/usr/lib/nls/C/deroff.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/lib/nls/C/deroff.cat
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
	! chmog 555 bin bin ../../../../Filesets.sl/
