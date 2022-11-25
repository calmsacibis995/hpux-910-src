# @(#) $Revision: 27.1.1.3 $       
ROOT= 

BIN=$(ROOT)/usr/bin

CFLAGS= -c

LDFLAGS= -s

LIBS= -lPW

all:	bdiff

default:        bdiff
bdiff:	bdiff.o
	$(CC) $(LDFLAGS) -o bdiff bdiff.o $(LIBS) 

bdiff.o:	bdiff.c
	$(CC) $(CFLAGS) bdiff.c

oldinstall:	release

release:	all
	cp  bdiff $(BIN)/bdiff
	chmod 555 $(BIN)/bdiff
	chgrp bin $(BIN)/bdiff
	chown bin $(BIN)/bdiff

clean:
	-rm -f bdiff.o

clobber:	clean
	-rm -f bdiff
# copy /usr/bin/bdiff to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/bdiff
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
install:
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp bdiff ../../../../Filesets.sl/CMDS-AUX/usr/bin/bdiff
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/bdiff
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
