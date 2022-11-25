default:	cut.c
	 $(CC)  cut.c -o cut
	 findmsg cut.c > cut.msg
	 gencat cut.cat cut.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f cut

# copy /usr/bin/cut to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/cut
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cut
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/bin
	! cp cut ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin/cut
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
# copy /usr/bin/cut to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/cut
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/bin
	! cp cut ../../../../Filesets.sl/UX-CORE/usr/bin/cut
	! chmog 0555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin/cut
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
