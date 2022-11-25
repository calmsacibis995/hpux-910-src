# $Source: /misc/source_product/9.10/commands.rcs/lib/libDUX/makefile,v $
# $Revision: 64.3.1.1 $

# Common makefile for libDUX

# Description file for libDUX: hidden kernel entry points for DISKLESS

LIBNAME = libDUX.a
OBJECTS = cluster.o csp.o

#----------------------------------------------------
# NOTE: this should not be delivered to the customer.
#----------------------------------------------------

default: all

all: $(OBJECTS)
	/bin/rm -f libDUX.a
	ar q $(LIBNAME) `lorder $(OBJECTS) | tsort`

clean:
	/bin/rm -f $(OBJECTS)

clobber: clean
	/bin/rm -f $(LIBNAME)


#
# Assemble a ".s" file with cpp directives in a machine independent
# manner
#
.s.o:
	/bin/rm -f _tmp.s
	$(CC) -E $(CFLAGS) $(FLAGS) $*.s >_tmp.s
	$(CC) -c $(CFLAGS) $(FLAGS) _tmp.s
	mv _tmp.o $*.o
	/bin/rm -f _tmp.s

install:
	cp libDUX.a /lib
