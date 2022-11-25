# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/bif/makefile,v $
# $Revision: 64.2.1.16 $

# Common makefile for bif commands

LIBS    = bif.o error.o misc.o

PROGS   = bifmkdir bifchmod bifchown bifchgrp bifrm   bifrmdir bifls    \
	  bifcp    biffind  bifmkfs  biffsdb  biffsck bifdf

default: all

all: $(PROGS)

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

clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f $(PROGS)
# copy /usr/bin/bif to the . relative fileset BIF-CMDS
# change mode, owner, and group on /usr/bin/bif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! cp bifchgrp ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifchgrp
	! chmog 0555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifchgrp
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/bif to the . relative fileset BIF-CMDS
# change mode, owner, and group on /usr/bin/bif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! cp bifchmod ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifchmod
	! chmog 0555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifchmod
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/bif to the . relative fileset BIF-CMDS
# change mode, owner, and group on /usr/bin/bif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! cp bifchown ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifchown
	! chmog 0555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifchown
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/bif to the . relative fileset BIF-CMDS
# change mode, owner, and group on /usr/bin/bif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! cp bifcp ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifcp
	! chmog 0555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifcp
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/bif to the . relative fileset BIF-CMDS
# change mode, owner, and group on /usr/bin/bif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! cp bifdf ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifdf
	! chmog 0555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifdf
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/bif to the . relative fileset BIF-CMDS
# change mode, owner, and group on /usr/bin/bif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! cp biffind ../../../../Filesets.sl/BIF-CMDS/usr/bin/biffind
	! chmog 0555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin/biffind
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/bif to the . relative fileset BIF-CMDS
# change mode, owner, and group on /usr/bin/bif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! cp biffsck ../../../../Filesets.sl/BIF-CMDS/usr/bin/biffsck
	! chmog 0555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin/biffsck
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/bif to the . relative fileset BIF-CMDS
# change mode, owner, and group on /usr/bin/bif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! cp biffsdb ../../../../Filesets.sl/BIF-CMDS/usr/bin/biffsdb
	! chmog 0555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin/biffsdb
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/bif to the . relative fileset BIF-CMDS
# change mode, owner, and group on /usr/bin/bif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! cp bifls ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifls
	! chmog 0555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifls
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/bif to the . relative fileset BIF-CMDS
# change mode, owner, and group on /usr/bin/bif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! cp bifmkdir ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifmkdir
	! chmog 0555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifmkdir
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/bif to the . relative fileset BIF-CMDS
# change mode, owner, and group on /usr/bin/bif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! cp bifmkfs ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifmkfs
	! chmog 0555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifmkfs
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/bif to the . relative fileset BIF-CMDS
# change mode, owner, and group on /usr/bin/bif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! cp bifrm ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifrm
	! chmog 0555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifrm
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/bif to the . relative fileset BIF-CMDS
# change mode, owner, and group on /usr/bin/bif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! cp bifrmdir ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifrmdir
	! chmog 0555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifrmdir
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/bif to the . relative fileset BIF-CMDS
# change mode, owner, and group on /usr/bin/bif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! ln ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifls ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifll
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/bif to the . relative fileset BIF-CMDS
# change mode, owner, and group on /usr/bin/bif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! ln ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifcp ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifln
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/bif to the . relative fileset BIF-CMDS
# change mode, owner, and group on /usr/bin/bif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! ln ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifcp ../../../../Filesets.sl/BIF-CMDS/usr/bin/bifmv
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BIF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
