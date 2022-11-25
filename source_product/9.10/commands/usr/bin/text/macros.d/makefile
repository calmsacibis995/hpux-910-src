# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/text/macros.d/makefile,v $
# $Revision: 66.2 $

# Common makefile for the text Developement Support Library (DSL) macros

RPATH 	 = /usr/bin

MACFILES = ntar an mmn osdd

default all: $(MACFILES)

# for ntar only the 300 formerly used cc.unshared
ntar.o: ntar.c
	$(CC) $(CFLAGS) -c ntar.c

ntar:	ntar.o
	$(CC) $(LDFLAGS) -Wl,-a,archive -o ntar ntar.o

mmn:	macrunch ntar
	sh ./macrunch -n m -d $(RPATH) mmn

osdd:	macrunch ntar
	sh ./macrunch -d $(RPATH) osdd

an:	macrunch ntar
	sh ./macrunch -n an -d $(RPATH) an

clean:
	/bin/rm -f *.o

clobber:  clean
	/bin/rm -f $(MACFILES) cmp.n.d.an cmp.n.d.m cmp.n.t.an \
		cmp.n.t.m ucmp.n.an ucmp.n.m
