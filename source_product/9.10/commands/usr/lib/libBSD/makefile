# $Source: /misc/source_product/9.10/commands.rcs/usr/lib/libBSD/makefile,v $
# $Revision: 66.2.1.2 $

# Common makefile for libBSD 
# This makefile also used to build libV7, which is now implemented as
# a link to libBSD.  This linking is done by the install portion of the
# common build.
#

# Temporarily removed tell.o from OBJECTS list so that 300 build will
# work.

OBJECTS = hpux_rel.o getwd.o killpg.o sigvec.o signal.o \
	setpgrp.o getpgrp.o

LIB	= libBSD.a

default:    $(LIB)

$(LIB):     $(OBJECTS)
	/bin/rm -f $(LIB)
	ar  q  $(LIB)  `lorder $(OBJECTS) | tsort`

clean:
	/bin/rm -f *.o

clobber:    clean
	/bin/rm -f $(LIB)
# copy /usr/lib/libBSD to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libBSD
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp libBSD.a ../../../../Filesets.sl/BUILDENV/usr/lib/libBSD.a
	! chmog 0444 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/libBSD.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libBSD to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libBSD
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! cp libBSD.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libBSD.a
	! chmog 0444 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/libBSD.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
