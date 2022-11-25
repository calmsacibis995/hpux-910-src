# $Header: makefile,v 66.2 90/07/31 11:26:47 rsh Exp $

# Common makefile for ci command of rcs

INC      = ../include
PFLAGS	 = -I$(INC)
LIBS	 = ../lib/librcs.a -lBUILD

default: all

all:	ci

ci:	ci.o ../hpux_rel.o
	$(CC) $(LDFLAGS) -o ci ../hpux_rel.o ci.o $(LIBS)

clean:		
	/bin/rm -f *.o 

clobber: clean
	/bin/rm -f ci 

ci.o:	$(INC)/system.h $(INC)/bin.h \
	$(INC)/rcsbase.h $(INC)/time.h

# Redefine the .c to .o rule as to include PFLAGS
.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c $<
