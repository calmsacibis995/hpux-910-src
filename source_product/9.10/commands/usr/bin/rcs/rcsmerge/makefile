# $Header: makefile,v 66.3 90/07/31 11:28:46 rsh Exp $

# Common makefile for rcsmerge command of rcs utility

INC	  = ../include
PFLAGS    = -I$(INC)
LIBS	  = ../lib/librcs.a -lBUILD

default:  rcsmerge

rcsmerge: rcsmerge.o ../hpux_rel.o
	  $(CC) $(LDFLAGS) rcsmerge.o ../hpux_rel.o $(LIBS) -o rcsmerge

clean:	
	  /bin/rm -f rcsmerge.o

clobber:  clean
	  /bin/rm -f rcsmerge

rcsmerge.o: $(INC)/system.h $(INC)/bin.h \
	    $(INC)/rcsbase.h $(INC)/copyright.h

# Redefine the .c to .o rule as to include PFLAGS
.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c $<
