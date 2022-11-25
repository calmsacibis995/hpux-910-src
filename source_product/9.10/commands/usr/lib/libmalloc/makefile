# $Source: /misc/source_product/9.10/commands.rcs/usr/lib/libmalloc/makefile,v $
# $Revision: 66.5.1.1 $

# Common makefile for libmalloc
# The old make files used to place the profiled files into a seperate
# directory.  This make file will just rename the profiled files by 
# replacing the .c with a _p.c.

OBJECTS = hpux_rel.o malloc.o
OBJECTS_P = $(OBJECTS:.o=_p.o)
OBJECTS_SL = $(OBJECTS:.o=_sl.o)
LIB	= libmalloc.a
LIB_P	= libmalloc_p.a
LIB_SL	= libmalloc.sl
LINT	= lint
PFLAGS	= -p
DFLAGS	= -DPROF

default: $(LIB) $(LIB_P)

$(LIB): $(OBJECTS)
	rm -f $(LIB)
	ar q $(LIB) `lorder $(OBJECTS) | tsort`

$(LIB_P): $(OBJECTS_P)
	rm -f $(LIB_P)
	ar q $(LIB_P) `lorder $(OBJECTS_P) | tsort`

$(LIB_SL): $(OBJECTS_SL)
	/bin/rm -f $(LIB_SL)
	ld -b -o $(LIB_SL) $(OBJECTS_SL)

malloc.o:	mallint.h

.c_p.o:
	$(CC) $(CFLAGS) $(PFLAGS) $(DFLAGS) -c $< -o $@

.c_sl.o:
	$(CC) $(CFLAGS) +z -c $< -o $@

clean: 
	/bin/rm -f $(OBJECTS) $(OBJECTS_P) $(OBJECTS_SL)

clobber:	 clean
	/bin/rm -f $(LIB) $(LIB_P) $(LIB_SL)

.SUFFIXES: _p.o _sl.o
# copy /usr/lib/libmalloc to the . relative fileset C-MIN
# change mode, owner, and group on /usr/lib/libmalloc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/C-MIN/usr/old
	! cp libmalloc.a ../../../../Filesets.sl/C-MIN/usr/old/libmalloc3x.a
	! chmog 0444 bin bin ../../../../Filesets.sl/C-MIN/usr/old/libmalloc3x.a
	! chmog 555 bin bin ../../../../Filesets.sl/C-MIN/usr/old
	! chmog 555 bin bin ../../../../Filesets.sl/C-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
