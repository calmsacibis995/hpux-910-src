# $Source: /misc/source_product/9.10/commands.rcs/bin/linkers/ld/makefile,v $
# $Revision: 70.1 $

# S300 makefile for ld command (linker)
# Cleaned up by Jeff Benjamin for the Common Build Process.
# Modified by Marc Sabatella
# Modified by Greg Lindhorst
# Modified by Mark Egeland to create an archived version of ld

#BBAFLAGS = -tp,/usr/hp64000/lib/bbacpp -DBBA_OPTA -DBBA_OPTO=A -DBBA
FLAGS = $(BBAFLAGS) -I /usr/contrib/include -DNDEBUG
CC = /bin/c89
LD = /bin/ld

ALTH = /usr/contrib/include

SOURCES = main.c pass1.c middle.c pass2.c deb.c sym.c emes.c shlib.c export.c \
          hpux_rel.c 
OBJECTS = $(SOURCES:.c=.o)
HEADERS = ld.defs.h funcs.h $(ALTH)/symtab.h $(ALTH)/dnttsizes.h


default: ld ld.arch

ld:  $(OBJECTS)
	$(CC) -o ld -q $(OBJECTS)

ld.arch: $(OBJECTS)
	$(CC) -o ld.arch -q -Wl,-a,archive $(OBJECTS)

bba:	$(OBJECTS) bba.a
	$(CC) -o ld -q $(OBJECTS) bba.a
	touch bba

debug:	$(OBJECTS)
	$(CC) -o ld -g $(OBJECTS)
	touch debug

chkmem:		chkmem.o $(OBJECTS)
	$(CC) -o ld $(OBJECTS) chkmem.o
	touch chkmem

slowmem:	ld.sl chkmem.o hook.o
	$(CC) -o ld chkmem.o hook.o ld.sl -Wl,-a,archive
	adb -w ld <flagit
	touch slowmem

ld.sl:	$(OBJECTS)
	$(LD) -b -o ld.sl $(OBJECTS)

bba.a:	$(OBJECTS)
	/usr/hp64000/bin/bbagen -d/tmp/ld/bbadump.data $(OBJECTS)

# Dependencies among source/object files:

$(OBJECTS):      $(HEADERS)

#
# Re-define .c.o rule so implicit compiles use FLAGS
#
.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<


clean:
	/bin/rm -f $(OBJECTS) chkmem.o bba.a

clobber:
	/bin/rm -f ld ld.arch debug bba chkmem slowmem ld.sl
