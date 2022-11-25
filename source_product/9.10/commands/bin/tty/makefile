default:	tty.c
	 $(CC)  tty.c -o tty
	 findmsg tty.c > tty.msg
	 gencat tty.cat tty.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f tty

# copy /bin/tty to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/tty
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	tty
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp tty ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/tty
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/tty to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/tty
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp tty ../../../Filesets.sl/UX-CORE/bin/tty
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/tty
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/tty to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/tty
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! ln ../../../Filesets.sl/UX-CORE/bin/tty ../../../Filesets.sl/UX-CORE/bin/pty
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
