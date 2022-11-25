# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/gprof/makefile,v $
# $Revision: 64.8 $

# s300 makefile for gprof.  Has been cleaned up to work in conjunction
# with the common build process.

GPROFHDRS =	gprof.h hp9000s300.h
GPROFSRCS =	gprof.c arcs.c dfn.c lookup.c calls.c \
		printgprof.c printlist.c
GPROFOBJS =	gprof.o arcs.o dfn.o lookup.o calls.o \
		printgprof.o printlist.o

default: gprof

gprof: $(GPROFOBJS)
	cc -o gprof $(LDFLAGS) $(CFLAGS) $(GPROFOBJS)


clean:
	/bin/rm -f $(GPROFOBJS) errs

clobber: clean
	/bin/rm -f gprof

# Dependencies:

gprof.o: gprof.c gprof.h hp9000s300.h gcrt0.h
arcs.o: arcs.c gprof.h hp9000s300.h gcrt0.h
lookup.o: lookup.c gprof.h hp9000s300.h gcrt0.h
dfn.o: dfn.c gprof.h hp9000s300.h gcrt0.h
calls.o: calls.c gprof.h hp9000s300.h gcrt0.h
printgprof.o: printgprof.c gprof.h hp9000s300.h gcrt0.h
printlist.o: printlist.c gprof.h hp9000s300.h gcrt0.h

# copy /usr/bin/gprof to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/gprof
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	gprof
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/bin
	! cp gprof ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin/gprof
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
# copy /usr/bin/gprof to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/gprof
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/bin
	! cp gprof ../../../../Filesets.sl/PROG-AUX/usr/bin/gprof
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin/gprof
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/gprof to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/gprof
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! cp gprof.callg ../../../../Filesets.sl/PROG-AUX/usr/lib/gprof.callg
	! chmog 0444 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/gprof.callg
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/gprof to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/gprof
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! cp gprof.flat ../../../../Filesets.sl/PROG-AUX/usr/lib/gprof.flat
	! chmog 0444 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/gprof.flat
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
