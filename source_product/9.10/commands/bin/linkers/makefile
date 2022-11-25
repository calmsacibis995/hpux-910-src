# $Source: /misc/source_product/9.10/commands.rcs/bin/linkers/dld/makefile,v $
# $Revision: 70.4 $

# S300 makefile for /lib/dld.sl (dynamic loader)
# Author: Marc Sabatella

.SUFFIXES:	.ln

CC = /bin/c89
M4 = /usr/bin/m4

COPTIONS = -D_HPUX_SOURCE +z -Wa,-i +O3 -Wi,-l100
FEATURES = -DELABORATOR -DHOOKS -DVISIBILITY -DBIND_FLAGS -DSEARCH_ORDER \
           -DNO_INTRA_FIX -DCHECK_HANDLE -DSHLIB_PATH -DSHLIB_PATH_LEN=1024 \
           -DPERROR -DGETHANDLE \
           -DFLUSH_CACHE -DCHEAT_IN_ASM \
           -DDLD
DEBUG =
BBA =
CBBA =

CFLGS = $(COPTIONS) $(FEATURES) $(DEBUG) $(BBA) $(CBBA)
MFLGS = $(FEATURES) $(DEBUG) $(BBA)
AFLGS = +z -i

ASOURCES = jump.s bor.s stack.s
CSOURCES = init.c load.c bind.c find.c export.c user.c hpux_rel.c
# cache.c packet.c no longer necessary

# important - jump.o must be the first object file
OBJECTS = $(ASOURCES:.s=.o) $(CSOURCES:.c=.o)
BBAOBJECTS = jump.o bor.o stack.o nobba.o load.o bind.o find.o \
             export.o user.o hpux_rel.o

HEADERS = dld.h

default:	dld.sl libdld.sl libdlda.o

dld.sl:		$(OBJECTS)
	$(LD) -i -s -o dld.sl $(OBJECTS) -lpicc

debug:		$(OBJECTS)
	$(LD) -i -o dld.sl $(OBJECTS) -lpicc
	touch debug

bba:		$(BBAOBJECTS) bba.a
	$(LD) -i -o dld.sl $(BBAOBJECTS) bba.a -lpicc
	touch bba

lint:		$(CSOURCES:.c=.ln)
	$(LINT) -ux $(CSOURCES:.c=.ln)
	touch lint

libdld.sl:	libdld.o shl_load.o
	$(LD) -b -o libdld.sl libdld.o shl_load.o

libdlda.o:	new.o old.o
	$(LD) -r -o libdlda.o new.o old.o

new.o:	shl_load.c
	$(CC) $(CFLAGS) $(FEATURES) +z -c -o new.o -DARCHIVE shl_load.c

old.o:	tmp.o
	$(LD) -r -o old.o -h _shl_load -h __shl_load tmp.o

tmp.o:	libdld.c
	$(CC) $(CFLAGS) $(FEATURES) +z -c -o tmp.o -DARCHIVE libdld.c

print:
	pr dld.h $(CSOURCES) $(ASOURCES) libdld.c shl_load.c makefile | lp -olp2 -depoc

shar:
	shar -h dld.h $(CSOURCES) $(ASOURCES) libdld.c shl_load.c makefile >source.shar

bba.a:		$(BBAOBJECTS)
	/usr/hp64000/bin/bbagen -v -C "+z -c" -b pic.a -d /source/linkers/dld/BBA/bbadump.data $(BBAOBJECTS)

nobba.o:	init.c
	$(CC) $(CFLAGS) $(COPTIONS) $(FEATURES) $(DEBUG) -DBBA -c -o nobba.o init.c

libdld.o:	libdld.c
	$(CC) $(CFLAGS) $(FEATURES) +z -c libdld.c

shl_load.o:	shl_load.c
	$(CC) $(CFLAGS) $(FEATURES) +z -c shl_load.c

$(CSOURCES:.c=.o):	$(HEADERS)

$(CSOURCES:.c=.ln):	$(HEADERS)

# Re-define .c.o rule so implicit compiles use CFLGS
# Same for .s.o; also use m4
#
.c.o:
	$(CC) $(CFLAGS) $(CFLGS) -c $<
.s.o:
	$(M4) $(MFLAGS) $(MFLGS) $<  | $(AS) $(AFLAGS) $(AFLGS) -o $*.o

.c.ln:
	$(LINT) -x -Aa $(CFLAGS) $(CFLGS) -c $<

clean:
	/bin/rm -f *.o *.M *.ln

clobber:
	/bin/rm -f *.sl libdlda.o bba debug lint
