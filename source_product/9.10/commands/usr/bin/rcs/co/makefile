# $Header: makefile,v 66.2 90/07/31 11:27:14 rsh Exp $

# Common makefile for co command in rcs

INC      = ../include
PFLAGS   = -I$(INC)
LIBS     = ../lib/librcs.a -lBUILD

default: all

all:	co

co:  	co.o ../hpux_rel.o
	$(CC) $(LDFLAGS) -o co ../hpux_rel.o co.o $(LIBS)

clean:		
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f co

co.o:	$(INC)/system.h $(INC)/bin.h \
	$(INC)/rcsbase.h $(INC)/time.h

# Redefine the .c to .o rule as to include PFLAGS
.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c $<
