# $Source: /misc/source_product/9.10/commands.rcs/usr/lib/libdbm/makefile,v $
# $Revision: 70.1.2.6 $

# Common makefile for libdbm

# This makefile formerly built libndbm, which was a link to libdbm.
# This link is now created by the install phase of the build process

OBJECTS     = hpux_rel.o dbm.o ndbm.o
LIB_SL	    = libdbm.sl
LIB_A	    = libdbm.a 
CFLAGS	    = +z


default: $(LIB_A) $(LIB_SL)

$(LIB_A):     	$(OBJECTS)
		/bin/rm -f $(LIB_A)
		ar q $(LIB_A) `lorder $(OBJECTS) | tsort`

$(LIB_SL):	$(OBJECTS)
		/bin/rm -f $(LIB_SL)
		$(LD) -o $(LIB_SL) -b $(OBJECTS)

clean:
		/bin/rm -f $(OBJECTS)

clobber:    	clean
		/bin/rm -f $(LIB_A) $(LIB_SL)
# copy /usr/lib/libdbm to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libdbm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp libdbm.a ../../../../Filesets.sl/BUILDENV/usr/lib/libdbm.a
	! chmog 0444 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/libdbm.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libdbm to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libdbm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! cp libdbm.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libdbm.a
	! chmog 0444 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/libdbm.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libdbm to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libdbm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! cp libdbm.sl ../../../../Filesets.sl/PROG-AUX/usr/lib/libdbm.sl
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/libdbm.sl
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libdbm to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libdbm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! ln ../../../../Filesets.sl/BUILDENV/usr/lib/libdbm.a ../../../../Filesets.sl/BUILDENV/usr/lib/libndbm.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libdbm to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libdbm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! ln ../../../../Filesets.sl/PROG-AUX/usr/lib/libdbm.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libndbm.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libdbm to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libdbm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! ln ../../../../Filesets.sl/PROG-AUX/usr/lib/libdbm.sl ../../../../Filesets.sl/PROG-AUX/usr/lib/libndbm.sl
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
