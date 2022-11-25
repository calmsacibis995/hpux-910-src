# @(#) $Revision: 64.1 $     

ROOT    =
INSDIR  = $(ROOT)/usr/bin
CFLAGS  = -O -DNLS -DNLS16
LDFLAGS = -s

LIBS    = bif.o error.o misc.o

PROGS   = bifmkdir bifchmod bifchown bifchgrp bifrm   bifrmdir bifls    \
	  bifcp    biffind  bifmkfs  biffsdb  biffsck bifdf

all: progs
progs:	$(PROGS)

bifmknod:	$(LIBS) bifmknod.o bif.h bif2.h dir.h
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o $(LIBS) -o $@

bifmkdir:	$(LIBS) bifmkdir.o bif.h bif2.h dir.h
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o $(LIBS) -o $@

bifchmod:	$(LIBS) bifchmod.o bif.h bif2.h dir.h
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o $(LIBS) -o $@

bifchown:	$(LIBS) bifchown.o bif.h bif2.h dir.h
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o $(LIBS) -o $@

bifchgrp:	$(LIBS) bifchgrp.o bif.h bif2.h dir.h
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o $(LIBS) -o $@

biftouch:	$(LIBS) biftouch.o bif.h bif2.h dir.h
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o $(LIBS) -o $@

bifcp:	$(LIBS) bifcp.o bif.h bif2.h dir.h
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o $(LIBS) -o $@

bifls:	$(LIBS) bifls.o bif.h bif2.h dir.h
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o $(LIBS) -o $@

bifrm:	$(LIBS) bifrm.o bif.h bif2.h dir.h
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o $(LIBS) -o $@

bifrmdir:	$(LIBS) bifrmdir.o bif.h bif2.h dir.h
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o $(LIBS) -o $@

biffind:	$(LIBS) biffind.o bif.h bif2.h dir.h
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o $(LIBS) -o $@

bifmkfs:	$(LIBS) bifmkfs.o bif.h bif2.h dir.h
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o -o $@

biffsck:	$(LIBS) biffsck.o bif.h bif2.h dir.h
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o -o $@

biffsdb:	$(LIBS) biffsdb.o bif.h bif2.h dir.h
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o -o $@

bifdf:		$(LIBS) bifdf.o bif.h bif2.h dir.h
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o $(LIBS) -o $@

bif.o:	bif.h

biffind.o bif.h: dir.h

release: all
	cp $(PROGS) $(INSDIR)
	cd $(INSDIR); chmod 555 $(PROGS)
	cd $(INSDIR); chgrp bin $(PROGS)
	cd $(INSDIR); chown bin $(PROGS)
	cd $(INSDIR); ln bifcp bifln; ln bifcp bifmv; ln bifls bifll

install: release

clean:
	rm -f *.o

clobber: clean
	rm -f $(PROGS)
