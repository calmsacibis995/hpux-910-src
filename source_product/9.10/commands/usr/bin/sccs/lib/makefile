# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/sccs/lib/mpwlib/makefile,v $
# $Revision: 64.3 $

# Common makefile for building mpwlib.a for sccs

LIB = ../mpwlib.a
HDR = ../../hdr

OBJECTS = fmalloc.o xcreat.o xmsg.o

default: $(LIB)

$(LIB): $(OBJECTS)
	/bin/rm -f $(LIB)
	/bin/ar q $(LIB) $(OBJECTS)

xcreat.o xmsg.o: $(HDR)/defines.h

clean:
	/bin/rm -f $(OBJECTS)

clobber: clean
	/bin/rm -f $(LIB)

.PRECIOUS:	$(LIB)
