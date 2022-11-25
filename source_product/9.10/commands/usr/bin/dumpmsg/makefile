default:	dumpmsg.c
	 $(CC)  dumpmsg.c -o dumpmsg
	 findmsg dumpmsg.c > dumpmsg.msg
	 gencat dumpmsg.cat dumpmsg.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f dumpmsg

# copy /usr/bin/dumpmsg to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/dumpmsg
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	dumpmsg
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp dumpmsg ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/dumpmsg
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
# copy /usr/bin/dumpmsg to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/dumpmsg
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp dumpmsg ../../../../Filesets.sl/NLS-CORE/usr/bin/dumpmsg
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/dumpmsg
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/dumpmsg to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/dumpmsg
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! cp dumpmsg.cat ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/dumpmsg.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/dumpmsg.cat
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
