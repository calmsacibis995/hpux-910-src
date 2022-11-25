default:	locale.c
	 $(CC)  locale.c -o locale
	 findmsg locale.c > locale.msg
	 gencat locale.cat locale.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f locale

# copy /usr/bin/locale to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/locale
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	locale
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp locale ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/locale
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
# copy /usr/bin/locale to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/locale
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp locale ../../../../Filesets.sl/NLS-CORE/usr/bin/locale
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/locale
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/locale to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/locale
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! cp locale.cat ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/locale.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/locale.cat
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
