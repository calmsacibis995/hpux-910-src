# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/ptx/makefile,v $
# $Revision: 64.2.1.5 $

# Common makefile for ptx
# The previous make files also did the linking of eign to eign.sh,
# this will be done by the build process.

OBJECTS	= ptx.o hpux_rel.o

default: ptx

ptx:	$(OBJECTS)
	$(CC) $(LDFLAGS) -o ptx $(OBJECTS)

clean:
	/bin/rm -f $(OBJECTS)

clobber: clean
	/bin/rm -f eign ptx

# copy /usr/bin/ptx to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/ptx
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	ptx
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp ptx ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/ptx
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
# copy /usr/bin/ptx to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/ptx
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! cp ptx ../../../../Filesets.sl/TEXT-FMT/usr/bin/ptx
	! chmog 0555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin/ptx
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/ptx to the . relative fileset TEXT-FMT
# change mode, owner, and group on /usr/bin/ptx
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TEXT-FMT/usr/lib
	! cp eign.sh ../../../../Filesets.sl/TEXT-FMT/usr/lib/eign
	! chmog 0444 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/lib/eign
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TEXT-FMT/
	! chmog 555 bin bin ../../../../Filesets.sl/
