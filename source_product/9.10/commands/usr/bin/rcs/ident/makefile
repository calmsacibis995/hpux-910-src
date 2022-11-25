# $Header: makefile,v 66.2 90/07/31 11:27:36 rsh Exp $

# Common makefile for ident command of rcs utility

INC      = ../include
PFLAGS   = -I$(INC)
LIBS     = ../lib/librcs.a -lBUILD

default: all

all:	ident

ident:	ident.o ../hpux_rel.o
	$(CC) $(LDFLAGS) ident.o ../hpux_rel.o $(LIBS) -o ident

clean:		
	/bin/rm -f *.o 

clobber: clean
	/bin/rm -f ident

ident.o: $(INC)/system.h $(INC)/rcsbase.h

# Redefine the .c to .o rule as to include PFLAGS
.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c $<
