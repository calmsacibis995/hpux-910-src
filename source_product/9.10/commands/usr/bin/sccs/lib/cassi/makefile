# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/sccs/lib/cassi/makefile,v $
# $Revision: 64.3 $

# Common makefile for building cassi.a library for sccs

LIB = ../cassi.a
HDR = ../../hdr

OBJECTS = gf.o cmrcheck.o deltack.o error.o abspath.o filehand.o

default: $(LIB)

$(LIB): $(OBJECTS)
	/bin/rm -f $(LIB)
	/bin/ar q $(LIB) $(OBJECTS)

# Dependencies
gf.o cmrcheck.o filehand.o: $(HDR)/filehand.h
deltack.o: $(HDR)/filehand.h $(HDR)/had.h $(HDR)/defines.h

clean:
	/bin/rm -f $(OBJECTS)

clobber:	clean
	/bin/rm -f $(LIB)

.PRECIOUS:	$(LIB)
