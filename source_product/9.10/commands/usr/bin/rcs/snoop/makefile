# $Header: makefile,v 66.3 90/07/31 11:29:25 rsh Exp $

# Common makefile for snoop command of rcs utility

INC	  = ../include
PFLAGS	  = -I$(INC)
LIBS	  = ../lib/librcs.a -lBUILD

default:  snoop.rcs

snoop.rcs:     snoop.o ../hpux_rel.o
	  $(CC) $(LDFLAGS) snoop.o ../hpux_rel.o $(LIBS) -o snoop.rcs

clean:		
	  /bin/rm -f snoop.o

clobber:  clean
	  /bin/rm -f snoop.rcs

snoop.o:  $(INC)/system.h $(INC)/bin.h \
	  $(INC)/rcsbase.h 

# Redefine the .c to .o rule as to include PFLAGS
.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c $<
