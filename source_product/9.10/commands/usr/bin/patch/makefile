# UNISRC_ID	@(#)37.4   87/02/20

LOCALDEST = /usr/contrib/bin

default:
	rm -f patch
	echo "#!/bin/sh"                                                >>patch
	echo "echo patch is not yet a supported command >&2"            >>patch
	echo "exit 1"                                                   >>patch
	chmod +x patch

patch: patch.c
	$(CC) patch.c -o patch

contrib: patch
	-cp	patch	$(LOCALDEST)/patch
	-chown	bin	$(LOCALDEST)/patch
	-chgrp	bin	$(LOCALDEST)/patch
	-chmod	755	$(LOCALDEST)/patch

oldinstall: patch
	$(INS) patch patch.1

clean:
	rm -f *.o *.out

clobber: clean
	rm -f patch lint

lint: patch.c
	lint patch.c > lint
# copy /usr/bin/patch to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/patch
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
install:
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp patch ../../../../Filesets.sl/CMDS-AUX/usr/bin/patch
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/patch
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
