# $Header: makefile,v 66.3 90/07/31 11:28:01 rsh Exp $

# Common makefile for rcs command

INC	= ../include
PFLAGS  = -I$(INC)
LIBS	= ../lib/librcs.a -lBUILD

default: rcs

rcs:    rcs.o ../hpux_rel.o
	$(CC) $(LDFLAGS) rcs.o ../hpux_rel.o $(LIBS) -o rcs

clean:		
	/bin/rm -f core rcs.o

clobber: clean
	/bin/rm -f rcs

rcs.o:  $(INC)/system.h $(INC)/bin.h \
	$(INC)/rcsbase.h $(INC)/copyright.h

# Redefine the .c to .o rule as to include PFLAGS
.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c $<
