default:	ttytype.c
	 mv ttytype ttytype.file
	 $(CC)  ttytype.c -o ttytype
	 $(CC)  -Wl,-a,archive ttytype.c -o ttytype.arch
	 findmsg ttytype.c > ttytype.msg
	 gencat ttytype.cat ttytype.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f ttytype
# copy /etc/ttytype to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/ttytype
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp ttytype ../../../Filesets.sl/UX-CORE/bin/ttytype
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/ttytype
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/ttytype to the . relative fileset INSTALL
# change mode, owner, and group on /etc/ttytype
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/INSTALL/etc
	! cp ttytype.arch ../../../Filesets.sl/INSTALL/etc/ttytype.arch
	! chmog 0555 bin bin ../../../Filesets.sl/INSTALL/etc/ttytype.arch
	! chmog 555 bin bin ../../../Filesets.sl/INSTALL/etc
	! chmog 555 bin bin ../../../Filesets.sl/INSTALL/
	! chmog 555 bin bin ../../../Filesets.sl/
	mkdir -p ../../../Filesets.sl/UX-CORE/etc/newconfig
	! cp ttytype.file ../../../Filesets.sl/UX-CORE/etc/newconfig/ttytype
	! chmog 444 bin bin ../../../Filesets.sl/UX-CORE/etc/newconfig/ttytype
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc/newconfig
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE
	! chmog 555 bin bin ../../../Filesets.sl
