
# HPUX_ID: @(#) $Revision: 70.1.1.3 $
# @(#) $Revision: 70.1.1.3 $       

ROOT=
CFLAGS= -DNLS -DNLS16
LDFLAGS= -s
FINDMSG=/usr/bin/findmsg
GENCAT=/usr/bin/gencat

default: findstr findstr.cat

findstr: findstr.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o findstr findstr.c -ll

findstr.c:	findstr.l
	$(LEX) -t findstr.l > findstr.c

findstr.cat:	findstr.c
	$(FINDMSG) findstr.c > findstr.msg
	$(GENCAT) $@ findstr.msg

clean:
	-rm -f findstr.o findstr.c findstr.msg

clobber: clean
	-rm -f findstr findstr.cat

oldinstall: release

release: findstr
	cp    findstr	$(ROOT)/usr/bin/findstr
	chmod 555	$(ROOT)/usr/bin/findstr
	chgrp bin	$(ROOT)/usr/bin/findstr
	chown bin	$(ROOT)/usr/bin/findstr
	installmsg	$(ROOT) findstr

install:
# copy /usr/bin/findstr to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/findstr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp findstr ../../../../Filesets.sl/NLS-CORE/usr/bin/findstr
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/findstr
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/findstr to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/findstr
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! cp findstr.cat ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/findstr.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/findstr.cat
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
