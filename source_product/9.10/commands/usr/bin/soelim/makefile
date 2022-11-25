default:	soelim.c
	 $(CC)  soelim.c -o soelim
	 findmsg soelim.c > soelim.msg
	 gencat soelim.cat soelim.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f soelim

# copy /usr/bin/soelim to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/soelim
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	soelim
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp soelim ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/soelim
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
# copy /usr/bin/soelim to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/soelim
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp soelim ../../../../Filesets.sl/TEXT-FMT/usr/bin/soelim
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/soelim
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
	! chmog 555 bin bin ../../../../Filesets.sl/
