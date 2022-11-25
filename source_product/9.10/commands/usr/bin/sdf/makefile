# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/sdf/makefile,v $
# $Revision: 64.2.1.15 $

# Common makefile for sdf utility (all of PROGS listed below)

# Lib sdf.a will be created using LIBOBJS.
LIB	= sdf.a

LIBOBJS	= open.o read.o stat.o ustat.o chown.o link.o mknod.o \
	  addr.o misc.o io.o   error.o

# All executables to be created.
PROGS	= sdfmkdir sdfchmod sdfchgrp sdfchown sdfrm sdfls sdfcp \
	  sdffind sdffsdb sdffsck sdfdf

default: all

all:	$(LIB) $(PROGS)

$(LIB): $(LIBOBJS)
	/bin/rm -f $(LIB)
	/bin/ar q $(LIB) $(LIBOBJS)

$(LIBOBJS): s500defs.h sdf.h

# Special case, will not look in sdf.a library file.  Creates executable
# from .c file.
sdffsck: sdffsck.c
	$(CC) $(CFLAGS) $(LDFLAGS) $@.c -o $@

addr.o ustat.o:	bit.h

clean:
	/bin/rm -f *.o *.a

clobber: clean
	/bin/rm -f $(PROGS)

# Redefine how to create .c file.  This is so that can include sdf.a library
# file.  Also creates executable following the creation of the object file.
.c:	$(LIB) $@.o
	$(CC) $(CFLAGS) $(LDFLAGS) -c $@.c
	$(CC) $(CFLAGS) $(LDFLAGS) $@.o $(LIB) -o $@
# copy /usr/bin/sdf to the . relative fileset SDF-CMDS
# change mode, owner, and group on /usr/bin/sdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! cp sdfchgrp ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfchgrp
	! chmog 0555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfchgrp
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/sdf to the . relative fileset SDF-CMDS
# change mode, owner, and group on /usr/bin/sdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! cp sdfchmod ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfchmod
	! chmog 0555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfchmod
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/sdf to the . relative fileset SDF-CMDS
# change mode, owner, and group on /usr/bin/sdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! cp sdfchown ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfchown
	! chmog 0555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfchown
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/sdf to the . relative fileset SDF-CMDS
# change mode, owner, and group on /usr/bin/sdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! cp sdfcp ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfcp
	! chmog 0555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfcp
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/sdf to the . relative fileset SDF-CMDS
# change mode, owner, and group on /usr/bin/sdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! cp sdfdf ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfdf
	! chmog 0555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfdf
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/sdf to the . relative fileset SDF-CMDS
# change mode, owner, and group on /usr/bin/sdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! cp sdffind ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdffind
	! chmog 0555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdffind
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/sdf to the . relative fileset SDF-CMDS
# change mode, owner, and group on /usr/bin/sdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! cp sdffsck ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdffsck
	! chmog 0555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdffsck
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/sdf to the . relative fileset SDF-CMDS
# change mode, owner, and group on /usr/bin/sdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! cp sdffsdb ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdffsdb
	! chmog 0555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdffsdb
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/sdf to the . relative fileset SDF-CMDS
# change mode, owner, and group on /usr/bin/sdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! cp sdfls ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfls
	! chmog 0555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfls
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/sdf to the . relative fileset SDF-CMDS
# change mode, owner, and group on /usr/bin/sdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! cp sdfmkdir ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfmkdir
	! chmog 0555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfmkdir
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/sdf to the . relative fileset SDF-CMDS
# change mode, owner, and group on /usr/bin/sdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! cp sdfrm ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfrm
	! chmog 0555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfrm
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/sdf to the . relative fileset SDF-CMDS
# change mode, owner, and group on /usr/bin/sdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! ln ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfls ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfll
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/sdf to the . relative fileset SDF-CMDS
# change mode, owner, and group on /usr/bin/sdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! ln ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfcp ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfln
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/sdf to the . relative fileset SDF-CMDS
# change mode, owner, and group on /usr/bin/sdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! ln ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfcp ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfmv
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/sdf to the . relative fileset SDF-CMDS
# change mode, owner, and group on /usr/bin/sdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! ln ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfrm ../../../../Filesets.sl/SDF-CMDS/usr/bin/sdfrmdir
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SDF-CMDS/
	! chmog 555 bin bin ../../../../Filesets.sl/
