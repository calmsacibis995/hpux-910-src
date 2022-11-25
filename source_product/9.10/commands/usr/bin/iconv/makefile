default:	iconv.c
	 $(CC)  iconv.c -o iconv
	 findmsg iconv.c > iconv.msg
	 gencat iconv.cat iconv.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f iconv

# copy /usr/bin/iconv to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/iconv
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	iconv
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp iconv ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/iconv
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
# copy /usr/bin/iconv to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/iconv
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp iconv ../../../../Filesets.sl/NLS-CORE/usr/bin/iconv
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/iconv
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/iconv to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/iconv
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! cp iconv.cat ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/iconv.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/iconv.cat
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
