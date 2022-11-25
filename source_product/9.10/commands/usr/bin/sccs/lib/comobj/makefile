# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/sccs/lib/comobj/makefile,v $
# $Revision: 64.3 $

# Common makefile for building the comobj.a library for sccs

LIB = ../comobj.a
HDR = ../../hdr

OBJECTS = auxf.o chkid.o chksid.o date_ab.o date_ba.o del_ab.o del_ba.o \
	  dodelt.o dofile.o dohist.o doie.o dolist.o eqsid.o flushto.o  \
	  fmterr.o getline.o logname.o newstats.o permiss.o pf_ab.o 	\
	  putline.o rdmod.o setup.o sid_ab.o sid_ba.o sidtoser.o 	\
	  sinit.o stats_ab.o

default: $(LIB)

$(LIB): $(OBJECTS)
	/bin/rm -f $(LIB)
	/bin/ar q $(LIB) $(OBJECTS)

# Dependencies
$(OBJECTS): $(HDR)/defines.h
dohist.o: $(HDR)/had.h

clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f $(LIB)

.PRECIOUS:	$(LIB)
