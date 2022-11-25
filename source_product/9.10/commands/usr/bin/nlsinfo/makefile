default:	nlsinfo.c
	 $(CC) +O1 nlsinfo.c -o nlsinfo
	 findmsg nlsinfo.c > nlsinfo.msg
	 gencat nlsinfo.cat nlsinfo.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f nlsinfo

# copy /usr/bin/nlsinfo to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/nlsinfo
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	nlsinfo
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp nlsinfo ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/nlsinfo
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
# copy /usr/bin/nlsinfo to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/nlsinfo
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp nlsinfo ../../../../Filesets.sl/NLS-CORE/usr/bin/nlsinfo
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/nlsinfo
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/nlsinfo to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/nlsinfo
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! cp nlsinfo.cat ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/nlsinfo.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/nlsinfo.cat
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
