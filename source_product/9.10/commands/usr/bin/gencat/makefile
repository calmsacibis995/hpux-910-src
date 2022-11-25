default:	gencat.c
	 $(CC)  gencat.c -o gencat
	 findmsg gencat.c > gencat.msg
	 gencat gencat.cat gencat.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f gencat

# copy /usr/bin/gencat to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/gencat
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	gencat
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp gencat ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/gencat
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
# copy /usr/bin/gencat to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/gencat
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp gencat ../../../../Filesets.sl/NLS-CORE/usr/bin/gencat
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/gencat
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/gencat to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/gencat
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! cp gencat.cat ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/gencat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/gencat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
