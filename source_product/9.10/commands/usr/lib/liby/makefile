# $Source: /misc/source_product/9.10/commands.rcs/usr/lib/liby/makefile,v $
# $Revision: 64.2.1.2 $

# Common makefile for liby
# The old makefile also did the linking to /usr/lib, this will be taken care
# of by the build process.

OBJECTS = hpux_rel.o libmai.o libzer.o
LIB	= liby.a

default:	$(LIB)

$(LIB):	$(OBJECTS)
	/bin/rm -f $(LIB)
	ar q $(LIB) `lorder $(OBJECTS) | tsort`

clean:
	/bin/rm -f *.o

clobber:	clean
	/bin/rm -f $(LIB) 
# copy /usr/lib/liby to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/liby
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp liby.a ../../../../Filesets.sl/BUILDENV/usr/lib/liby.a
	! chmog 0444 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/liby.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/liby to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/lib/liby
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp liby.a ../../../../Filesets.sl/C-TOOLS/usr/lib/liby.a
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/liby.a
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
