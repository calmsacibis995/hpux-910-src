default:	insertmsg.c
	 $(CC)  insertmsg.c -o insertmsg
	 findmsg insertmsg.c > insertmsg.msg
	 gencat insertmsg.cat insertmsg.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f insertmsg

# copy /usr/bin/insertmsg to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/insertmsg
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	insertmsg
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp insertmsg ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/insertmsg
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
# copy /usr/bin/insertmsg to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/insertmsg
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp insertmsg ../../../../Filesets.sl/NLS-CORE/usr/bin/insertmsg
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/insertmsg
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/insertmsg to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/insertmsg
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! cp insertmsg.cat ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/insertmsg.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/insertmsg.cat
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
