# @(#) $Revision: 27.4.1.3 $      
#	factor make file

ROOT =
INSDIR = $(ROOT)/usr/bin
CFLAGS = -O
LDFLAGS = -s
SOURCE = factor.c

default:	factor
factor:	$(SOURCE)
	$(CC) $(CFLAGS) $(LDFLAGS) -o factor factor.c -lm

oldinstall: release

release:  factor
	cp    factor $(INSDIR)/factor
	chmod 555    $(INSDIR)/factor
	chgrp bin    $(INSDIR)/factor
	chown bin    $(INSDIR)/factor

clean:
	rm -f factor.o

clobber:	clean
	  rm -f factor
# copy /usr/bin/factor to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/factor
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
install:
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp factor ../../../../Filesets.sl/CMDS-AUX/usr/bin/factor
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/factor
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
