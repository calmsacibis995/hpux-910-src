# $Source: /misc/source_product/9.10/commands.rcs/usr/lib/libempty/makefile,v $
# $Revision: 64.1.1.18 $

# makefile for libempty
#
#

# This Makefile creates a dummy library, libempty.a.  At the time of this
# writing, libndir, libbsdipc and libnet link to it.  The only purpose of
# this dummy library is to prevent old Makefiles from breaking.

LIB=	libempty.a

OBJECTS=  hpux_rel.o

default: $(LIB)

$(LIB): $(OBJECTS)
	/bin/rm -f libempty.a
	ar q libempty.a $(OBJECTS)

clean:
	/bin/rm -f $(OBJECTS)

clobber:    clean
	/bin/rm -f libempty.a
# copy /usr/lib/libempty to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp libempty.a ../../../../Filesets.sl/BUILDENV/usr/lib/libempty.a
	! chmog 0444 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/libempty.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! cp libempty.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libempty.a
	! chmog 0444 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/libempty.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! ln ../../../../Filesets.sl/BUILDENV/usr/lib/libempty.a ../../../../Filesets.sl/BUILDENV/usr/lib/libV7.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! ln ../../../../Filesets.sl/PROG-AUX/usr/lib/libempty.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libV7.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! ln ../../../../Filesets.sl/BUILDENV/usr/lib/libempty.a ../../../../Filesets.sl/BUILDENV/usr/lib/libbsdipc.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! ln ../../../../Filesets.sl/PROG-AUX/usr/lib/libempty.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libbsdipc.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! ln ../../../../Filesets.sl/BUILDENV/usr/lib/libempty.a ../../../../Filesets.sl/BUILDENV/usr/lib/libcrypt.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! ln ../../../../Filesets.sl/PROG-AUX/usr/lib/libempty.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libcrypt.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! ln ../../../../Filesets.sl/BUILDENV/usr/lib/libempty.a ../../../../Filesets.sl/BUILDENV/usr/lib/libmalloc.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! ln ../../../../Filesets.sl/PROG-AUX/usr/lib/libempty.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libmalloc.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! ln ../../../../Filesets.sl/BUILDENV/usr/lib/libempty.a ../../../../Filesets.sl/BUILDENV/usr/lib/libn.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! ln ../../../../Filesets.sl/PROG-AUX/usr/lib/libempty.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libn.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! ln ../../../../Filesets.sl/BUILDENV/usr/lib/libempty.a ../../../../Filesets.sl/BUILDENV/usr/lib/libndir.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! ln ../../../../Filesets.sl/PROG-AUX/usr/lib/libempty.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libndir.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! ln ../../../../Filesets.sl/BUILDENV/usr/lib/libempty.a ../../../../Filesets.sl/BUILDENV/usr/lib/libnet.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! ln ../../../../Filesets.sl/PROG-AUX/usr/lib/libempty.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libnet.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! ln ../../../../Filesets.sl/BUILDENV/usr/lib/libempty.a ../../../../Filesets.sl/BUILDENV/usr/lib/libnsipc.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libempty to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libempty
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! ln ../../../../Filesets.sl/PROG-AUX/usr/lib/libempty.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libnsipc.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
