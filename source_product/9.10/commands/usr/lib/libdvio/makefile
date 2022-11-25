# $Source: /misc/source_product/9.10/commands.rcs/usr/lib/libdvio/makefile,v $
# @(#) $Revision: 66.4 $    

KERNOPTS = -Dhp9000s200 -Dhpux -Uvax -DMOREFILES -DHFS
FLAGS = $(KERNOPTS)

OBJECTS = dillib.o dillibs.o hpux_rel.o

default:    libdvio.a

libdvio.a:	$(OBJECTS)
	/bin/rm -f libdvio.a
	ar q libdvio.a `lorder $(OBJECTS) | tsort`

dillibs.o:	assym.s dillibs.s	
	cat assym.s dillibs.s >dillibs.c
	$(AS) -o dillibs.o dillibs.c
	rm -f dillibs.c

assym.s:	dillibsym.o
	$(CC) -Wl,-a,archive dillibsym.o
	./a.out > assym.s
	rm -f a.out

clean:
	/bin/rm -f $(OBJECTS) assym.s dillibsym.o

clobber:    clean
	/bin/rm -f libdvio.a

# 
# Re-define .c.o rule so that implicit compiles from source to object
# use FLAGS
#
.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<
# copy /usr/lib/libdvio to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libdvio
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp libdvio.a ../../../../Filesets.sl/BUILDENV/usr/lib/libdvio.a
	! chmog 0444 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/libdvio.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libdvio to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libdvio
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! cp libdvio.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libdvio.a
	! chmog 0444 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/libdvio.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
