default:	findmsg.c
	 $(CC)  findmsg.c -o findmsg
	 findmsg findmsg.c > findmsg.msg
	 gencat findmsg.cat findmsg.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f findmsg

# copy /usr/bin/findmsg to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/findmsg
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	findmsg
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp findmsg ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/findmsg
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
# copy /usr/bin/findmsg to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/findmsg
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp findmsg ../../../../Filesets.sl/NLS-CORE/usr/bin/findmsg
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/findmsg
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/findmsg to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/findmsg
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! cp findmsg.cat ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/findmsg.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/findmsg.cat
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
